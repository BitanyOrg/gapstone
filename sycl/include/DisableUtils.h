#ifndef DISABLE_UTILS_H
#define DISABLE_UTILS_H

#undef LLVM_DEBUG
#define LLVM_DEBUG(msg) ;
#undef llvm_unreachable
#define llvm_unreachable(msg) ;

#endif