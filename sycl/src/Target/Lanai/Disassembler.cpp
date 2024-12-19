#include "Lanai/LanaiSyclDisassembler.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include <range.hpp>
#include <sycl/sycl.hpp>
#include <usm.hpp>
#include <vector>
#include "Lanai/Decode.h"
#include "DecodeInstruction.h"

namespace gapstone {

namespace LanaiImpl {
static DecodeStatus
disassemble_instruction(MCInstGPU_Lanai &MI, ArrayRef<uint8_t> Bytes,
                        uint64_t Address, const FeatureBitset &Bits) {
  uint32_t Insn;

  DecodeStatus Result = readInstruction32(Bytes, MI.Size, Insn);

  if (Result == MCDisassembler::Fail)
    return MCDisassembler::Fail;

  // Call auto-generated decoder function
  Result =
      decodeInstruction(DecoderTableLanai32, MI, Insn, Address, nullptr, Bits);

  if (Result != MCDisassembler::Fail) {
    PostOperandDecodeAdjust(MI, Insn);
    MI.Size = 4;
    return Result;
  }

  return MCDisassembler::Fail;
}
#include "DisassembleImpl.h"
} // namespace LanaiImpl
std::unique_ptr<InstInfoContainer> LanaiDisassembler::batch_disassemble(
    uint64_t base_addr, std::vector<uint8_t> &content, int step_size) {
  return LanaiImpl::disassemble_impl<MCInstGPU_Lanai>(
      q, MCDisassembler, base_addr, content, step_size);
}
} // namespace gapstone