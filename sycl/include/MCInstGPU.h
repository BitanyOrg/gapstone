//===- llvm/MC/MCInstGPU.h - MCInstGPU class --------------------------*- C++
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the MCInstGPU and MCOperandGPU classes,
// which is the basic representation used to represent low-level machine code
// instructions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_MC_MCINSTGPU_H
#define LLVM_MC_MCINSTGPU_H

#include "StaticVector.h"
#include <cassert>
#include <llvm/ADT/bit.h>
#include <llvm/MC/MCInst.h>
#include <llvm/Support/SMLoc.h>

namespace llvm {

/// Instances of this class represent a single low-level machine
/// instruction.
template <unsigned N = 8, typename InsnType = uint64_t> class MCInstGPU {

public:
  unsigned Opcode = 0;
  // These flags could be used to pass some info from one target subcomponent
  // to another, for example, from disassembler to asm printer. The values of
  // the flags have any sense on target level only (e.g. prefixes on x86).
  unsigned Flags = 0;
  unsigned DecodeIdx;
  unsigned ActualNumOperands;
  unsigned Size;
  unsigned TableIdx; // Used for ARM Decoding
  InsnType Insn;
  SMLoc Loc;
  StaticVector<MCOperand, N> Operands;

  MCInstGPU() = default;

  void setOpcode(unsigned Op) { Opcode = Op; }
  unsigned getOpcode() const { return Opcode; }

  void setFlags(unsigned F) { Flags = F; }
  unsigned getFlags() const { return Flags; }

  void setLoc(SMLoc loc) { Loc = loc; }
  SMLoc getLoc() const { return Loc; }

  const MCOperand &getOperand(unsigned i) const { return Operands[i]; }
  MCOperand &getOperand(unsigned i) { return Operands[i]; }
  unsigned getNumOperands() const { return Operands.size(); }

  void addOperand(const MCOperand Op) {
    Operands.push_back(Op);
    ++ActualNumOperands;
  }

  using iterator = typename StaticVector<MCOperand, N>::iterator;
  using const_iterator = typename StaticVector<MCOperand, N>::const_iterator;

  void clear() { Operands.clear(); }
  size_t size() const { return Operands.size(); }
  iterator begin() { return Operands.begin(); }
  const_iterator begin() const { return Operands.begin(); }
  iterator end() { return Operands.end(); }
  const_iterator end() const { return Operands.end(); }

  iterator insert(iterator I, const MCOperand &Op) {
    return Operands.insert(I, Op);
  }
};

} // end namespace llvm

#endif // LLVM_MC_MCINSTGPU_H
