#include "llvm/MC/MCInst.h"
using namespace llvm;
static bool getMCRDeprecationInfo(MCInst &MI, const MCSubtargetInfo &STI,
                                  std::string &Info) {
  if (STI.hasFeature(llvm::ARM::HasV7Ops) &&
      (MI.getOperand(0).isImm() && MI.getOperand(0).getImm() == 15) &&
      (MI.getOperand(1).isImm() && MI.getOperand(1).getImm() == 0) &&
      // Checks for the deprecated CP15ISB encoding:
      // mcr p15, #0, rX, c7, c5, #4
      (MI.getOperand(3).isImm() && MI.getOperand(3).getImm() == 7)) {
    if ((MI.getOperand(5).isImm() && MI.getOperand(5).getImm() == 4)) {
      if (MI.getOperand(4).isImm() && MI.getOperand(4).getImm() == 5) {
        Info = "deprecated since v7, use 'isb'";
        return true;
      }

      // Checks for the deprecated CP15DSB encoding:
      // mcr p15, #0, rX, c7, c10, #4
      if (MI.getOperand(4).isImm() && MI.getOperand(4).getImm() == 10) {
        Info = "deprecated since v7, use 'dsb'";
        return true;
      }
    }
    // Checks for the deprecated CP15DMB encoding:
    // mcr p15, #0, rX, c7, c10, #5
    if (MI.getOperand(4).isImm() && MI.getOperand(4).getImm() == 10 &&
        (MI.getOperand(5).isImm() && MI.getOperand(5).getImm() == 5)) {
      Info = "deprecated since v7, use 'dmb'";
      return true;
    }
  }
  if (STI.hasFeature(llvm::ARM::HasV7Ops) &&
      ((MI.getOperand(0).isImm() && MI.getOperand(0).getImm() == 10) ||
       (MI.getOperand(0).isImm() && MI.getOperand(0).getImm() == 11))) {
    Info = "since v7, cp10 and cp11 are reserved for advanced SIMD or floating "
           "point instructions";
    return true;
  }
  return false;
}

static bool getMRCDeprecationInfo(MCInst &MI, const MCSubtargetInfo &STI,
                                  std::string &Info) {
  if (STI.hasFeature(llvm::ARM::HasV7Ops) &&
      ((MI.getOperand(0).isImm() && MI.getOperand(0).getImm() == 10) ||
       (MI.getOperand(0).isImm() && MI.getOperand(0).getImm() == 11))) {
    Info = "since v7, cp10 and cp11 are reserved for advanced SIMD or floating "
           "point instructions";
    return true;
  }
  return false;
}

static bool getARMStoreDeprecationInfo(MCInst &MI, const MCSubtargetInfo &STI,
                                       std::string &Info) {
  assert(!STI.hasFeature(llvm::ARM::ModeThumb) &&
         "cannot predicate thumb instructions");

  assert(MI.getNumOperands() >= 4 && "expected >= 4 arguments");
  for (unsigned OI = 4, OE = MI.getNumOperands(); OI < OE; ++OI) {
    assert(MI.getOperand(OI).isReg() && "expected register");
    if (MI.getOperand(OI).getReg() == ARM::PC) {
      Info = "use of PC in the list is deprecated";
      return true;
    }
  }
  return false;
}

static bool getARMLoadDeprecationInfo(MCInst &MI, const MCSubtargetInfo &STI,
                                      std::string &Info) {
  assert(!STI.hasFeature(llvm::ARM::ModeThumb) &&
         "cannot predicate thumb instructions");

  assert(MI.getNumOperands() >= 4 && "expected >= 4 arguments");
  bool ListContainsPC = false, ListContainsLR = false;
  for (unsigned OI = 4, OE = MI.getNumOperands(); OI < OE; ++OI) {
    assert(MI.getOperand(OI).isReg() && "expected register");
    switch (MI.getOperand(OI).getReg()) {
    default:
      break;
    case ARM::LR:
      ListContainsLR = true;
      break;
    case ARM::PC:
      ListContainsPC = true;
      break;
    }
  }

  if (ListContainsPC && ListContainsLR) {
    Info = "use of LR and PC simultaneously in the list is deprecated";
    return true;
  }

  return false;
}