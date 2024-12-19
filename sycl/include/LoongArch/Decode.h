//===-- LoongArchDisassembler.cpp - Disassembler for LoongArch ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the LoongArchDisassembler class.
//
//===----------------------------------------------------------------------===//

#include "DisableUtils.h"
#include "MCTargetDesc/LoongArchBaseInfo.h"
#include "MCTargetDesc/LoongArchMCTargetDesc.h"
#include "TargetInfo/LoongArchTargetInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDecoderOps.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Endian.h"
#include "MCInstGPU.h"

using namespace llvm;
using MCInstGPU_LoongArch = MCInstGPU<6>;
#define MCInst MCInstGPU_LoongArch

#define DEBUG_TYPE "loongarch-disassembler"

typedef MCDisassembler::DecodeStatus DecodeStatus;

static DecodeStatus DecodeGPRRegisterClass(MCInst &Inst, uint64_t RegNo,
                                           uint64_t Address,
                                           const MCDisassembler *Decoder) {
  if (RegNo >= 32)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createReg(LoongArch::R0 + RegNo));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeFPR32RegisterClass(MCInst &Inst, uint64_t RegNo,
                                             uint64_t Address,
                                             const MCDisassembler *Decoder) {
  if (RegNo >= 32)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createReg(LoongArch::F0 + RegNo));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeFPR64RegisterClass(MCInst &Inst, uint64_t RegNo,
                                             uint64_t Address,
                                             const MCDisassembler *Decoder) {
  if (RegNo >= 32)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createReg(LoongArch::F0_64 + RegNo));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeCFRRegisterClass(MCInst &Inst, uint64_t RegNo,
                                           uint64_t Address,
                                           const MCDisassembler *Decoder) {
  if (RegNo >= 8)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createReg(LoongArch::FCC0 + RegNo));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeFCSRRegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const MCDisassembler *Decoder) {
  if (RegNo >= 4)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createReg(LoongArch::FCSR0 + RegNo));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeLSX128RegisterClass(MCInst &Inst, uint64_t RegNo,
                                              uint64_t Address,
                                              const MCDisassembler *Decoder) {
  if (RegNo >= 32)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createReg(LoongArch::VR0 + RegNo));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeLASX256RegisterClass(MCInst &Inst, uint64_t RegNo,
                                               uint64_t Address,
                                               const MCDisassembler *Decoder) {
  if (RegNo >= 32)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createReg(LoongArch::XR0 + RegNo));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeSCRRegisterClass(MCInst &Inst, uint64_t RegNo,
                                           uint64_t Address,
                                           const MCDisassembler *Decoder) {
  if (RegNo >= 4)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createReg(LoongArch::SCR0 + RegNo));
  return MCDisassembler::Success;
}

template <unsigned N, int P = 0>
static DecodeStatus decodeUImmOperand(MCInst &Inst, uint64_t Imm,
                                      int64_t Address,
                                      const MCDisassembler *Decoder) {
  assert(isUInt<N>(Imm) && "Invalid immediate");
  Inst.addOperand(MCOperand::createImm(Imm + P));
  return MCDisassembler::Success;
}

template <unsigned N, unsigned S = 0>
static DecodeStatus decodeSImmOperand(MCInst &Inst, uint64_t Imm,
                                      int64_t Address,
                                      const MCDisassembler *Decoder) {
  assert(isUInt<N>(Imm) && "Invalid immediate");
  // Shift left Imm <S> bits, then sign-extend the number in the bottom <N+S>
  // bits.
  Inst.addOperand(MCOperand::createImm(SignExtend64<N + S>(Imm << S)));
  return MCDisassembler::Success;
}

#include "LoongArchGenDisassemblerTables.inc"
