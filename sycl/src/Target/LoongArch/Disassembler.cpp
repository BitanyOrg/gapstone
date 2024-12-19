#include "LoongArch/LoongArchSyclDisassembler.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/Support/Endian.h"
#include <range.hpp>
#include <sycl/sycl.hpp>
#include <usm.hpp>
#include "LoongArch/Decode.h"
#include "DecodeInstruction.h"

using namespace llvm;
namespace gapstone {

namespace LoongArchImpl {
static DecodeStatus disassemble_instruction(MCInstGPU_LoongArch &MI,
                                            ArrayRef<uint8_t> Bytes,
                                            uint64_t Address,
                                            const FeatureBitset &Bits) {
  uint32_t Insn;
  DecodeStatus Result;

  // We want to read exactly 4 bytes of data because all LoongArch instructions
  // are fixed 32 bits.
  if (Bytes.size() < 4) {
    MI.Size = 0;
    return MCDisassembler::Fail;
  }

  Insn = support::endian::read32le(Bytes.data());
  // Calling the auto-generated decoder function.
  Result = decodeInstruction(DecoderTable32, MI, Insn, Address, nullptr, Bits);
  MI.Size = 4;

  return Result;
}
#include "DisassembleImpl.h"
} // namespace LoongArchImpl
std::unique_ptr<InstInfoContainer> LoongArchDisassembler::batch_disassemble(
    uint64_t base_addr, std::vector<uint8_t> &content, int step_size) {
  return LoongArchImpl::disassemble_impl<MCInstGPU_LoongArch>(
      q, MCDisassembler, base_addr, content, step_size);
}
} // namespace gapstone