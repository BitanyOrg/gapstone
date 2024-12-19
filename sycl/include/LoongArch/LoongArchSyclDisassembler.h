#ifndef GAPSTONE_LOONGARCH_DISASSEMBLER_H
#define GAPSTONE_LOONGARCH_DISASSEMBLER_H
#include "SyclDisassembler.h"

namespace gapstone {
class LoongArchDisassembler : public SyclDisassembler {
public:
  LoongArchDisassembler(llvm::MCDisassembler &dd, sycl::queue &qq)
      : SyclDisassembler(dd, qq) {}
  virtual std::unique_ptr<InstInfoContainer> batch_disassemble(uint64_t base_addr,
                                             std::vector<uint8_t> &content,
                                             int step_size = 4) override;
};
} // namespace gapstone

#endif // GAPSTONE_LOONGARCH_DISASSEMBLER_H