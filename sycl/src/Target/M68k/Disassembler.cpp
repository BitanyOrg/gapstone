#include "M68k/M68kSyclDisassembler.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/Support/Endian.h"
#include <range.hpp>
#include <sycl/sycl.hpp>
#include <usm.hpp>
#include "M68k/Decode.h"

using namespace llvm;
namespace gapstone {

namespace M68kImpl {

void makeUp(uint64_t &Insn, unsigned InstrBits, ArrayRef<uint8_t> &Bytes) {
  unsigned Idx = (InstrBits + 7) >> 3; // Calculate the index in bytes
  unsigned RoundUp = alignTo(InstrBits, Align(16));
  if (RoundUp > 64) {
    return; // For now, just return
  }
  RoundUp = RoundUp >> 3; // Convert to bytes
  for (; Idx < RoundUp; Idx += 2) {
    // Insert 16 bits at a time
    uint16_t value = support::endian::read16be(&Bytes[Idx]);
    Insn |= static_cast<uint64_t>(value) << ((Idx * 8) % 64);
  }
}
#include "DecodeInstructionVarlen.h"

static DecodeStatus disassemble_instruction(MCInstGPU_M68k &MI,
                                            ArrayRef<uint8_t> &Bytes,
                                            uint64_t Address,
                                            const FeatureBitset &Bits) {
  DecodeStatus Result;
  MI.Insn = support::endian::read16be(Bytes.data());
  // 2 bytes of data are consumed, so set Size to 2
  // If we don't do this, disassembler may generate result even
  // the encoding is invalid. We need to let it fail correctly.
  MI.Size = 2;
  Result = decodeInstruction<uint64_t, MCInstGPU_M68k>(
      DecoderTable80, MI, MI.Insn, Address, nullptr, Bits, Bytes);
  if (Result == DecodeStatus::Success)
    MI.Size = InstrLenTable[MI.getOpcode()] >> 3;
  return Result;
}
#include "DisassembleImpl.h"
} // namespace M68kImpl
std::unique_ptr<InstInfoContainer> M68kDisassembler::batch_disassemble(
    uint64_t base_addr, std::vector<uint8_t> &content, int step_size) {
  return M68kImpl::disassemble_impl<MCInstGPU_M68k>(
      q, MCDisassembler, base_addr, content, step_size);
}
} // namespace gapstone