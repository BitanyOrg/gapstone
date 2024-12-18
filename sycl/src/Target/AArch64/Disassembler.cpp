#include "AArch64/AArch64SyclDisassembler.h"
#include "AArch64/Decode.h"
#include "DecodeInstruction.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include <range.hpp>
#include <sycl/sycl.hpp>
#include <usm.hpp>
#include <vector>
namespace gapstone {

namespace AArch64Impl {
static DecodeStatus disassemble_instruction(MCInstGPU_AArch64 &MI,
                                            uint64_t &Size,
                                            ArrayRef<uint8_t> Bytes,
                                            uint64_t Address,
                                            const FeatureBitset &Bits) {
  Size = 0;
  // We want to read exactly 4 bytes of data.
  if (Bytes.size() < 4)
    return MCDisassembler::Fail;
  Size = 4;

  // Encoded as a small-endian 32-bit word in the stream.
  unsigned Insn =
      (Bytes[3] << 24) | (Bytes[2] << 16) | (Bytes[1] << 8) | (Bytes[0] << 0);

  const uint8_t *Tables[] = {DecoderTable32, DecoderTableFallback32};
  for (const auto *Table : Tables) {
    DecodeStatus Result =
        decodeInstruction(Table, MI, Insn, Address, nullptr, Bits);

    if (Result != MCDisassembler::Fail)
      return Result;
  }

  return MCDisassembler::Fail;
}

#include "DisassembleImpl.h"
} // namespace AArch64Impl

std::unique_ptr<InstInfoContainer> AArch64Disassembler::batch_disassemble(
    uint64_t base_addr, std::vector<uint8_t> &content, int step_size) {
  return AArch64Impl::disassemble_impl<MCInstGPU_AArch64>(
      q, MCDisassembler, base_addr, content, step_size);
}
} // namespace gapstone