#include "AArch64/Decode.h"
#include "AArch64/GapstoneDisassembler.h"
#include "DecodeInstruction.h"
#include "MCInstGPU.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include <sycl/sycl.hpp>
#include <usm.hpp>
#include <vector>

// template <unsigned long N>
DecodeStatus disassemble_instruction(MCInstGPU &MI, unsigned &DecodeIdx,
                                     uint32_t &Insn, uint64_t &Size,
                                     ArrayRef<uint8_t> Bytes, uint64_t Address,
                                     const MCDisassembler *DisAsm,
                                     const FeatureBitset &Bits) {
  Size = 0;
  // We want to read exactly 4 bytes of data.
  if (Bytes.size() < 4)
    return MCDisassembler::Fail;
  Size = 4;

  // Encoded as a small-endian 32-bit word in the stream.
  Insn =
      (Bytes[3] << 24) | (Bytes[2] << 16) | (Bytes[1] << 8) | (Bytes[0] << 0);

  const uint8_t *Tables[] = {DecoderTable32, DecoderTableFallback32};
  for (const auto *Table : Tables) {
    DecodeStatus Result =
        decodeInstruction(Table, MI, DecodeIdx, Insn, Address, DisAsm, Bits);

    if (Result != MCDisassembler::Fail)
      return Result;
  }

  return MCDisassembler::Fail;
}

namespace gapstone {

std::vector<llvm::MCInst> disassemble_impl(sycl::queue &q,
                                           llvm::MCDisassembler &MCDisassembler,
                                           uint64_t base_addr,
                                           std::vector<uint8_t> &content,
                                           int step_size) {
  auto tasks = content.size() / step_size;
  llvm::MCInstGPU *results = sycl::malloc_host<llvm::MCInstGPU>(tasks, q);
  unsigned *decodeIdx = sycl::malloc_shared<unsigned>(tasks, q);
  uint32_t *insns = sycl::malloc_shared<uint32_t>(tasks, q);
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
      uint64_t offset = i * buffer_size / tasks;
      llvm::ArrayRef<uint8_t> array_ref(shared_content + offset, 4);
      status[i] = disassemble_instruction(results[i], decodeIdx[i], insns[i], size, array_ref,
                              base_addr + offset, nullptr, Bits);
    });
  });
  q.wait();
  std::vector<llvm::MCInst> insts(tasks);
  for (int i = 0; i < tasks; ++i) {
    bool DecodeComplete = true;
    insts[i].setOpcode(results[i].getOpcode());
    decodeToMCInst(status[i], decodeIdx[i], insns[i], insts[i], base_addr + 4 *i, &MCDisassembler, DecodeComplete);
  }
  return insts;
}

std::vector<llvm::MCInst> AArch64Disassembler::batch_disassemble(
    uint64_t base_addr, std::vector<uint8_t> &content, int step_size) {
  return disassemble_impl(q, MCDisassembler, base_addr, content, step_size);
}
} // namespace gapstone