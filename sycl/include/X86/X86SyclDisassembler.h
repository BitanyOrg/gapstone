#ifndef GAPSTONE_X86_DISASSEMBLER_H
#define GAPSTONE_X86_DISASSEMBLER_H
#include "SyclDisassembler.h"

namespace gapstone {
class X86Disassembler : public SyclDisassembler {
public:
  X86Disassembler(llvm::MCDisassembler &dd, sycl::queue &qq)
      : SyclDisassembler(dd, qq) {}
  virtual std::vector<InstInfo>
  batch_disassemble(uint64_t base_addr, std::vector<uint8_t> &content,
                    int step_size = 1) override;
};

} // namespace gapstone

#endif // GAPSTONE_X86_DISASSEMBLER_H