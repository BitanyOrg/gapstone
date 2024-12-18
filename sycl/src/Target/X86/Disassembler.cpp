#include "Disassembler/X86DisassemblerDecoder.h"
#include "X86/Decode.h"
#include "X86/X86SyclDisassembler.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/X86DisassemblerDecoderCommon.h"
#include <sycl/sycl.hpp>
#include <vector>
using DecodeStatus = llvm::MCDisassembler::DecodeStatus;

namespace gapstone {

namespace X86Impl {
template <unsigned N>
static DecodeStatus disassemble_instruction(MCInstGPU<N> &Instr, uint64_t &Size,
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

template <unsigned N>
static std::vector<InstInfo>
disassemble_impl(sycl::queue &q, llvm::MCDisassembler &MCDisassembler,
                 uint64_t base_addr, std::vector<uint8_t> &content,
                 int step_size) {
  auto tasks = content.size() / step_size;
  MCInstGPU<N> *gpu_insts = sycl::malloc_shared<MCInstGPU<N>>(tasks, q);
  DecodeStatus *status = sycl::malloc_shared<DecodeStatus>(tasks, q);
  uint8_t *shared_content = sycl::malloc_shared<uint8_t>(content.size(), q);
  q.memcpy(shared_content, content.data(), content.size());
  q.wait();
  const FeatureBitset &Bits =
      MCDisassembler.getSubtargetInfo().getFeatureBits();
  auto buffer_size = content.size();
  q.submit([&](sycl::handler &h) {
    h.parallel_for(tasks, [=](sycl::id<1> i) {
      // access shared_array and host_array on device
      uint64_t size;
      uint64_t offset = i * step_size;
      llvm::ArrayRef<uint8_t> array_ref(shared_content + offset,
                                        buffer_size - offset);
      status[i] = disassemble_instruction(gpu_insts[i], size, array_ref,
                                          base_addr + offset, Bits);
    });
  });
  q.wait();
  std::vector<InstInfo> insts(tasks);
  for (int i = 0; i < tasks; ++i) {
    if (gpu_insts[i].ActualNumOperands != gpu_insts[i].getNumOperands()) {
      std::cout << "Operands overflow for address " << std::hex
                << base_addr + i * step_size << std::endl;
      std::cout << "Actual: " << gpu_insts[i].ActualNumOperands << " Getting "
                << gpu_insts[i].getNumOperands() << std::endl;
    }
    insts[i].inst.setOpcode(gpu_insts[i].getOpcode());
    for (auto &operand : gpu_insts[i]) {
      insts[i].inst.addOperand(operand);
    }
    insts[i].status = status[i];
  }
  return insts;
}
} // namespace X86Impl

std::vector<InstInfo> X86Disassembler::batch_disassemble(
    uint64_t base_addr, std::vector<uint8_t> &content, int step_size) {
  return X86Impl::disassemble_impl<10>(q, MCDisassembler, base_addr, content,
                                       step_size);
}
} // namespace gapstone