#ifndef GAPSTONE_SYCL_DISASSEMBLER_H
#define GAPSTONE_SYCL_DISASSEMBLER_H

#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <sycl/sycl.hpp>
#include <vector>


namespace gapstone {

struct InstInfo {
  llvm::MCDisassembler::DecodeStatus status;
  llvm::MCInst inst;
};
class SyclDisassembler {
protected:
  llvm::MCDisassembler &MCDisassembler;
  sycl::queue &q;

public:
  SyclDisassembler(llvm::MCDisassembler &dd, sycl::queue &qq)
      : MCDisassembler(dd), q(qq){}
  virtual ~SyclDisassembler() = default;

  virtual std::vector<InstInfo>
  batch_disassemble(uint64_t base_addr, std::vector<uint8_t> &content,
                    int step_size = 1) = 0;
};
} // namespace gapstone

#endif // GAPSTONE_SYCL_DISASSEMBLER_H