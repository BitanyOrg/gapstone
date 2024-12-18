#include "Disassemblers.h"
#include "AArch64/AArch64SyclDisassembler.h"
#include "X86/X86SyclDisassembler.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/TargetParser/Triple.h"
#include <memory>

namespace gapstone {
std::unique_ptr<SyclDisassembler> createDisassembler(llvm::MCDisassembler &dd, sycl::queue &qq) {
    auto triple = dd.getSubtargetInfo().getTargetTriple();
    switch (triple.getArch()) {
        case llvm::Triple::ArchType::aarch64:
        std::cout << "Creating AArch64 Disassembler" << std::endl;
            return std::make_unique<AArch64Disassembler>(dd, qq);
        case llvm::Triple::x86_64:
        std::cout << "Creating X86 Disassembler" << std::endl;
            return std::make_unique<X86Disassembler>(dd, qq);
        default:
        throw "Not implemented yet";
    }
}  
} // namespace gapstone
