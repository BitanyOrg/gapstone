# gapstone
GPU accelerated disassembler

## Setup
Download llvm src from github, install the corresponding version of llvm.
For example, install llvm-18 for [llvm-18](https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.8/llvm-project-18.1.8.src.tar.xz)

Decompress the src code into the forder `llvm`.

Use the following scripts to dump json tblgen.
```bash
python3 scripts/generate_tables.py --tblgen llvm-tblgen-{LLVM_VERSION} --llvm ./llvm --output ./tblgen
```

## TODO

- [x] Decode Operands on Accelerators
- [] Architectures
    - [x] AArch64
    - [] AMDGPU
    - [] ARC
    - [] ARM (Unhealthy)
    - [] AVR
    - [] BPF
    - [] CSKY
    - [] DirectX
    - [] Hexagon
    - [] Lanai
    - [] LoongArch
    - [] M68k
    - [] Mips
    - [] MSP430
    - [] NVPTX
    - [] PowerPC
    - [] RISCV
    - [] Sparc
    - [] SPIRV
    - [] SystemZ
    - [] VE
    - [] WebAssembly
    - [x] X86
    - [] XCore
    - [] Xtensa
