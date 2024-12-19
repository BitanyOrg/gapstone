#include "Disassemblers.h"
#include "AArch64/AArch64SyclDisassembler.h"
#include "Lanai/LanaiSyclDisassembler.h"
#include "LoongArch/LoongArchSyclDisassembler.h"
#include "X86/X86SyclDisassembler.h"
#include "M68k/M68kSyclDisassembler.h"
// #include "ARM/ARMSyclDisassembler.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/TargetParser/Triple.h"
#include <memory>
#include <stdexcept>

namespace gapstone {
std::unique_ptr<SyclDisassembler> createDisassembler(llvm::MCDisassembler &dd,
                                                     sycl::queue &qq) {
  auto triple = dd.getSubtargetInfo().getTargetTriple();
  if (triple.isAArch64()) {
    std::cout << "Creating AArch64 Disassembler" << std::endl;
    return std::make_unique<AArch64Disassembler>(dd, qq);
  } else if (triple.isX86()) {
    std::cout << "Creating X86 Disassembler" << std::endl;
    return std::make_unique<X86Disassembler>(dd, qq);
  } else if (triple.isLoongArch()) {
    std::cout << "Creating LoongArch Disassembler" << std::endl;
    return std::make_unique<LoongArchDisassembler>(dd, qq);
  } else if (triple.getArch() == llvm::Triple::lanai) {
    std::cout << "Creating Lanai Disassembler" << std::endl;
    return std::make_unique<LanaiDisassembler>(dd, qq);
  } else if (triple.getArch() == llvm::Triple::m68k) {
    std::cout << "Creating M68k Disassembler" << std::endl;
    return std::make_unique<M68kDisassembler>(dd, qq);
  }
  // } else if (triple.isARM()) {
  //   std::cout << "Creating ARM Disassembler" << std::endl;
  //   return std::make_unique<ARMDisassembler>(dd, qq);
  else {
    throw std::invalid_argument("Not implemented yet");
  }
}
} // namespace gapstone
