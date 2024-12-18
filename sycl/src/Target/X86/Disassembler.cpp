#include "Disassembler/X86DisassemblerDecoder.h"
#include "SyclDisassembler.h"
#include "X86/Decode.h"
#include "X86/X86SyclDisassembler.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/Support/X86DisassemblerDecoderCommon.h"
#include <access/access.hpp>
#include <accessor.hpp>
#include <memory>
#include <sycl/sycl.hpp>
#include <vector>
using DecodeStatus = llvm::MCDisassembler::DecodeStatus;

namespace gapstone {

namespace X86Impl {
static DecodeStatus disassemble_instruction(MCInstGPU_X86 &Instr,
                                            uint64_t &Size,
                                            ArrayRef<uint8_t> Bytes,
                                            uint64_t Address,
                                            const FeatureBitset &Bits) {
  DisassemblerMode fMode;
  if (Bits[X86::Is16Bit]) {
    fMode = MODE_16BIT;
  } else if (Bits[X86::Is32Bit]) {
    fMode = MODE_32BIT;
  } else if (Bits[X86::Is64Bit]) {
    fMode = MODE_64BIT;
  } else {
    return MCDisassembler::Fail;
  }
  InternalInstruction Insn;
  memset(&Insn, 0, sizeof(InternalInstruction));
  Insn.bytes = Bytes;
  Insn.startLocation = Address;
  Insn.readerCursor = Address;
  Insn.mode = fMode;

  if (Bytes.empty() || readPrefixes(&Insn) || readOpcode(&Insn) ||
      getInstructionID(&Insn) || Insn.instructionID == 0 ||
      readOperands(&Insn)) {
    Size = Insn.readerCursor - Address;
    return MCDisassembler::Fail;
  }

  Insn.operands = x86OperandSets[Insn.spec->operands];
  Insn.length = Insn.readerCursor - Insn.startLocation;
  Size = Insn.length;
  if (Size > 15)
    LLVM_DEBUG(dbgs() << "Instruction exceeds 15-byte limit");

  bool Ret = translateInstruction(Instr, Insn, nullptr);
  if (!Ret) {
    unsigned Flags = X86::IP_NO_PREFIX;
    if (Insn.hasAdSize)
      Flags |= X86::IP_HAS_AD_SIZE;
    if (!Insn.mandatoryPrefix) {
      if (Insn.hasOpSize)
        Flags |= X86::IP_HAS_OP_SIZE;
      if (Insn.repeatPrefix == 0xf2)
        Flags |= X86::IP_HAS_REPEAT_NE;
      else if (Insn.repeatPrefix == 0xf3 &&
               // It should not be 'pause' f3 90
               Insn.opcode != 0x90)
        Flags |= X86::IP_HAS_REPEAT;
      if (Insn.hasLockPrefix)
        Flags |= X86::IP_HAS_LOCK;
    }
    Instr.setFlags(Flags);
  }
  return (!Ret) ? DecodeStatus::Success : DecodeStatus::Fail;
}

#include "DisassembleImpl.h"
} // namespace X86Impl

std::unique_ptr<InstInfoContainer> X86Disassembler::batch_disassemble(
    uint64_t base_addr, std::vector<uint8_t> &content, int step_size) {
  return X86Impl::disassemble_impl<MCInstGPU_X86>(q, MCDisassembler, base_addr,
                                                  content, step_size);
}
} // namespace gapstone