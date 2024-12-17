#ifndef GAPSTONE_DISASSEMBLERS_H
#define GAPSTONE_DISASSEMBLERS_H
#include "SyclDisassembler.h"
namespace gapstone {
extern void InitializeAllGapstoneDisassemblers();
extern std::unique_ptr<SyclDisassembler> createDisassembler(llvm::MCDisassembler &dd, sycl::queue &qq);
} // namespace gapstone

#endif // GAPSTONE_DISASSEMBLERS_H