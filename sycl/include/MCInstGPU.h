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
#include <cstdint>
#include <llvm/ADT/bit.h>
#include <llvm/Support/SMLoc.h>

namespace llvm {

class MCExpr;
class MCInstGPU;
class MCInstPrinter;
class MCRegisterInfo;
class raw_ostream;

/// Instances of this class represent operands of the MCInstGPU class.
/// This is a simple discriminated union.
class MCOperandGPU {
  enum MachineOperandType : unsigned char {
    kInvalid,      ///< Uninitialized.
    kRegister,     ///< Register operand.
    kImmediate,    ///< Immediate operand.
    kSFPImmediate, ///< Single-floating-point immediate operand.
    kDFPImmediate, ///< Double-Floating-point immediate operand.
    kExpr,         ///< Relocatable immediate operand.
    kInst          ///< Sub-instruction operand.
  };
  MachineOperandType Kind = kInvalid;

  union {
    unsigned RegVal;
    int64_t ImmVal;
    uint32_t SFPImmVal;
    uint64_t FPImmVal;
    const MCExpr *ExprVal;
    const MCInstGPU *InstVal;
  };

public:
  MCOperandGPU() : FPImmVal(0) {}

  bool isValid() const { return Kind != kInvalid; }
  bool isReg() const { return Kind == kRegister; }
  bool isImm() const { return Kind == kImmediate; }
  bool isSFPImm() const { return Kind == kSFPImmediate; }
  bool isDFPImm() const { return Kind == kDFPImmediate; }
  bool isExpr() const { return Kind == kExpr; }
  bool isInst() const { return Kind == kInst; }

  /// Returns the register number.
  unsigned getReg() const {
    assert(isReg() && "This is not a register operand!");
    return RegVal;
  }

  /// Set the register number.
  void setReg(unsigned Reg) {
    assert(isReg() && "This is not a register operand!");
    RegVal = Reg;
  }

  int64_t getImm() const {
    assert(isImm() && "This is not an immediate");
    return ImmVal;
  }

  void setImm(int64_t Val) {
    assert(isImm() && "This is not an immediate");
    ImmVal = Val;
  }

  uint32_t getSFPImm() const {
    assert(isSFPImm() && "This is not an SFP immediate");
    return SFPImmVal;
  }

  void setSFPImm(uint32_t Val) {
    assert(isSFPImm() && "This is not an SFP immediate");
    SFPImmVal = Val;
  }

  uint64_t getDFPImm() const {
    assert(isDFPImm() && "This is not an FP immediate");
    return FPImmVal;
  }

  void setDFPImm(uint64_t Val) {
    assert(isDFPImm() && "This is not an FP immediate");
    FPImmVal = Val;
  }
  void setFPImm(double Val) {
    assert(isDFPImm() && "This is not an FP immediate");
    FPImmVal = bit_cast<uint64_t>(Val);
  }

  const MCExpr *getExpr() const {
    assert(isExpr() && "This is not an expression");
    return ExprVal;
  }

  void setExpr(const MCExpr *Val) {
    assert(isExpr() && "This is not an expression");
    ExprVal = Val;
  }

  const MCInstGPU *getInst() const {
    assert(isInst() && "This is not a sub-instruction");
    return InstVal;
  }

  void setInst(const MCInstGPU *Val) {
    assert(isInst() && "This is not a sub-instruction");
    InstVal = Val;
  }

  static MCOperandGPU createReg(unsigned Reg) {
    MCOperandGPU Op;
    Op.Kind = kRegister;
    Op.RegVal = Reg;
    return Op;
  }

  static MCOperandGPU createImm(int64_t Val) {
    MCOperandGPU Op;
    Op.Kind = kImmediate;
    Op.ImmVal = Val;
    return Op;
  }

  static MCOperandGPU createSFPImm(uint32_t Val) {
    MCOperandGPU Op;
    Op.Kind = kSFPImmediate;
    Op.SFPImmVal = Val;
    return Op;
  }

  static MCOperandGPU createDFPImm(uint64_t Val) {
    MCOperandGPU Op;
    Op.Kind = kDFPImmediate;
    Op.FPImmVal = Val;
    return Op;
  }

  static MCOperandGPU createExpr(const MCExpr *Val) {
    MCOperandGPU Op;
    Op.Kind = kExpr;
    Op.ExprVal = Val;
    return Op;
  }

  static MCOperandGPU createInst(const MCInstGPU *Val) {
    MCOperandGPU Op;
    Op.Kind = kInst;
    Op.InstVal = Val;
    return Op;
  }

  void print(raw_ostream &OS, const MCRegisterInfo *RegInfo = nullptr) const;
  void dump() const;
  bool isBareSymbolRef() const;
  bool evaluateAsConstantImm(int64_t &Imm) const;
};

/// Instances of this class represent a single low-level machine
/// instruction.
class MCInstGPU {

public:
  unsigned Opcode = 0;
  // These flags could be used to pass some info from one target subcomponent
  // to another, for example, from disassembler to asm printer. The values of
  // the flags have any sense on target level only (e.g. prefixes on x86).
  unsigned Flags = 0;

  SMLoc Loc;
  StaticVector<MCOperandGPU, 10> Operands;

  MCInstGPU() = default;

  void setOpcode(unsigned Op) { Opcode = Op; }
  unsigned getOpcode() const { return Opcode; }

  void setFlags(unsigned F) { Flags = F; }
  unsigned getFlags() const { return Flags; }

  void setLoc(SMLoc loc) { Loc = loc; }
  SMLoc getLoc() const { return Loc; }

  const MCOperandGPU &getOperand(unsigned i) const { return Operands[i]; }
  MCOperandGPU &getOperand(unsigned i) { return Operands[i]; }
  unsigned getNumOperands() const { return Operands.size(); }

  void addOperand(const MCOperandGPU Op) { Operands.push_back(Op); }

  // using iterator = StaticVector<MCOperandGPU, 8>::iterator;
  // using const_iterator = StaticVector<MCOperandGPU, 8>::const_iterator;

  // void clear() { Operands.clear(); }
  // void erase(iterator I) { Operands.erase(I); }
  // void erase(iterator First, iterator Last) { Operands.erase(First, Last); }
  // size_t size() const { return Operands.size(); }
  // iterator begin() { return Operands.begin(); }
  // const_iterator begin() const { return Operands.begin(); }
  // iterator end() { return Operands.end(); }
  // const_iterator end() const { return Operands.end(); }

  // iterator insert(iterator I, const MCOperandGPU &Op) {
  //   return Operands.insert(I, Op);
  // }
};

} // end namespace llvm

#endif // LLVM_MC_MCINSTGPU_H
