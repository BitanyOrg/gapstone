//===-- M68kDisassembler.cpp - Disassembler for M68k ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is part of the M68k Disassembler.
//
//===----------------------------------------------------------------------===//

#include "M68k/M68k.h"
#include "M68k/M68kRegisterInfo.h"
#include "M68k/M68kSubtarget.h"
#include "M68k/MCTargetDesc/M68kMCCodeEmitter.h"
#include "M68k/MCTargetDesc/M68kMCTargetDesc.h"
#include "M68k/TargetInfo/M68kTargetInfo.h"

#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCDecoderOps.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/ErrorHandling.h"
#include "MCInstGPU.h"
#include "DisableUtils.h"


using namespace llvm;
using MCInstGPU_M68k = MCInstGPU<6>;
#define MCInst MCInstGPU_M68k

#define DEBUG_TYPE "m68k-disassembler"

typedef MCDisassembler::DecodeStatus DecodeStatus;

static const unsigned RegisterDecode[] = {
    M68k::D0,    M68k::D1,  M68k::D2,  M68k::D3,  M68k::D4,  M68k::D5,
    M68k::D6,    M68k::D7,  M68k::A0,  M68k::A1,  M68k::A2,  M68k::A3,
    M68k::A4,    M68k::A5,  M68k::A6,  M68k::SP,  M68k::FP0, M68k::FP1,
    M68k::FP2,   M68k::FP3, M68k::FP4, M68k::FP5, M68k::FP6, M68k::FP7,
    M68k::FPIAR, M68k::FPS, M68k::FPC};

static DecodeStatus DecodeRegisterClass(MCInst &Inst, uint64_t RegNo,
                                        uint64_t Address, const void *Decoder) {
  if (RegNo >= 24)
    return DecodeStatus::Fail;
  Inst.addOperand(MCOperand::createReg(RegisterDecode[RegNo]));
  return DecodeStatus::Success;
}

static DecodeStatus DecodeDR32RegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return DecodeRegisterClass(Inst, RegNo, Address, Decoder);
}

static DecodeStatus DecodeDR16RegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return DecodeRegisterClass(Inst, RegNo, Address, Decoder);
}

static DecodeStatus DecodeDR8RegisterClass(MCInst &Inst, uint64_t RegNo,
                                           uint64_t Address,
                                           const void *Decoder) {
  return DecodeRegisterClass(Inst, RegNo, Address, Decoder);
}

static DecodeStatus DecodeAR32RegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return DecodeRegisterClass(Inst, RegNo | 8ULL, Address, Decoder);
}

static DecodeStatus DecodeAR16RegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return DecodeRegisterClass(Inst, RegNo | 8ULL, Address, Decoder);
}

static DecodeStatus DecodeXR32RegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return DecodeRegisterClass(Inst, RegNo, Address, Decoder);
}

static DecodeStatus DecodeXR16RegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return DecodeRegisterClass(Inst, RegNo, Address, Decoder);
}

static DecodeStatus DecodeFPDRRegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return DecodeRegisterClass(Inst, RegNo | 16ULL, Address, Decoder);
}
#define DecodeFPDR32RegisterClass DecodeFPDRRegisterClass
#define DecodeFPDR64RegisterClass DecodeFPDRRegisterClass
#define DecodeFPDR80RegisterClass DecodeFPDRRegisterClass

static DecodeStatus DecodeFPCSCRegisterClass(MCInst &Inst, uint64_t RegNo,
                                             uint64_t Address,
                                             const void *Decoder) {
  return DecodeRegisterClass(Inst, (RegNo >> 1) + 24, Address, Decoder);
}
#define DecodeFPICRegisterClass DecodeFPCSCRegisterClass

static DecodeStatus DecodeCCRCRegisterClass(MCInst &Inst, uint64_t &Insn,
                                            uint64_t Address,
                                            const void *Decoder) {
  llvm_unreachable("unimplemented");
}

static DecodeStatus DecodeImm32(MCInst &Inst, uint64_t Imm, uint64_t Address,
                                const void *Decoder) {
  Inst.addOperand(MCOperand::createImm(M68k::swapWord<uint32_t>(Imm)));
  return DecodeStatus::Success;
}

#include "M68kGenDisassemblerTable.inc"

#undef DecodeFPDR32RegisterClass
#undef DecodeFPDR64RegisterClass
#undef DecodeFPDR80RegisterClass
#undef DecodeFPICRegisterClass

