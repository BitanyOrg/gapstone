# gapstone
GPU accelerated disassembler

## Setup
Install the requirements following guide from [CodePlay](https://developer.codeplay.com/products/oneapi/nvidia/2025.0.0/guides/get-started-guide-nvidia.html).

Clone this module and the submodules.
```bash
git clone https://github.com/5c4lar/gapstone.git
cd gapstone
git submodule update --init --recursive
```
Or
```bash
git clone --recursive https://github.com/5c4lar/gapstone.git
```
## Build

```bash
cmake -GNinja -Bbuild -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE -DWITHCUDA=True -DCMAKE_BUILD_TYPE=Release 
-DCMAKE_CXX_COMPILER=icpx .
ninja -C build
```

## TODO

- [x] Decode Operands on Accelerators
- [ ] Architectures
  - [x] X86
  - [x] AArch64
  - [x] Lanai
  - [ ] ARM (Unhealthy)
  - [ ] AMDGPU
  - [ ] ARC
  - [ ] AVR
  - [ ] BPF
  - [ ] CSKY
  - [ ] DirectX
  - [ ] Hexagon
  - [ ] LoongArch
  - [ ] M68k
  - [ ] Mips
  - [ ] MSP430
  - [ ] NVPTX
  - [ ] PowerPC
  - [ ] RISCV
  - [ ] Sparc
  - [ ] SPIRV
  - [ ] SystemZ
  - [ ] VE
  - [ ] WebAssembly
  - [ ] XCore
  - [ ] Xtensa
