#ifndef GAPSTONE_AArch64_DISASSEMBLER_H
#define GAPSTONE_AArch64_DISASSEMBLER_H
#include "SyclDisassembler.h"

namespace gapstone {
class AArch64Disassembler : public SyclDisassembler {
public:
  AArch64Disassembler(llvm::MCDisassembler &dd, sycl::queue &qq)
      : SyclDisassembler(dd, qq) {}
  virtual std::unique_ptr<InstInfoContainer> batch_disassemble(uint64_t base_addr,
                                             std::vector<uint8_t> &content,
                                             int step_size = 4) override;
};
} // namespace gapstone

#endif // GAPSTONE_AArch64_DISASSEMBLER_H