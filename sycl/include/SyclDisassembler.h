#ifndef GAPSTONE_SYCL_DISASSEMBLER_H
#define GAPSTONE_SYCL_DISASSEMBLER_H

#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInst.h>
#include <sycl/sycl.hpp>

namespace gapstone {

struct InstInfoContainer {
  uint64_t size;
  std::vector<llvm::MCDisassembler::DecodeStatus> status;
  InstInfoContainer(uint64_t n): status(std::vector<llvm::MCDisassembler::DecodeStatus>(n)) {}
  virtual llvm::MCInst getMCInst(uint64_t i) = 0;
  virtual ~InstInfoContainer() = default;
};

struct InstInfoContainerCPU : InstInfoContainer {
  std::vector<llvm::MCInst> insts;
  InstInfoContainerCPU(uint64_t n): InstInfoContainer(n), insts(std::vector<llvm::MCInst>(n)) {} 
  llvm::MCInst getMCInst(uint64_t i) override { return insts[i]; };
};

template <typename T> struct InstInfoContainerGPU : InstInfoContainer {
  std::vector<T> insts;
  InstInfoContainerGPU(uint64_t n): InstInfoContainer(n), insts(std::vector<T>(n)) {} 
  llvm::MCInst getMCInst(uint64_t i) override {
    llvm::MCInst res;
    res.setOpcode(insts[i].getOpcode());
    res.setFlags(insts[i].getFlags());
    res.setLoc(insts[i].getLoc());
    for (auto &operand : insts[i]) {
      res.addOperand(operand);
    }
    return res;
  };
};

class SyclDisassembler {
protected:
  llvm::MCDisassembler &MCDisassembler;
  sycl::queue &q;

public:
  SyclDisassembler(llvm::MCDisassembler &dd, sycl::queue &qq)
      : MCDisassembler(dd), q(qq) {}
  virtual ~SyclDisassembler() = default;

  virtual std::unique_ptr<InstInfoContainer> batch_disassemble(uint64_t base_addr,
                                      std::vector<uint8_t> &content,
                                      int step_size = 1) = 0;
};
} // namespace gapstone

#endif // GAPSTONE_SYCL_DISASSEMBLER_H