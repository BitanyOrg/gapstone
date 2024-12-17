#include "MCInstGPU.h"
#include <llvm/MC/MCDecoderOps.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/Support/LEB128.h>
#include <llvm/TargetParser/SubtargetFeature.h>
using DecodeStatus = llvm::MCDisassembler::DecodeStatus;
using namespace llvm;
// static bool checkDecoderPredicate(unsigned Idx, const FeatureBitset &Bits);



template <typename InsnType>
DecodeStatus decodeInstruction(const uint8_t DecodeTable[], MCInstGPU &MI, unsigned &DecodeIdx,
                               InsnType insn, uint64_t Address, const MCDisassembler *DisAsm,
                               const FeatureBitset &Bits) {
  const uint8_t *Ptr = DecodeTable;
  uint64_t CurFieldValue = 0;
  DecodeStatus S = MCDisassembler::Success;
  while (true) {
    // ptrdiff_t Loc = Ptr - DecodeTable;
    switch (*Ptr) {
    default:
      return MCDisassembler::Fail;
    case MCD::OPC_ExtractField: {
      unsigned Start = *++Ptr;
      unsigned Len = *++Ptr;
      ++Ptr;
      CurFieldValue = fieldFromInstruction(insn, Start, Len);
      break;
    }
    case MCD::OPC_FilterValue: {
      // Decode the field value.
      unsigned Len;
      uint64_t Val = decodeULEB128(++Ptr, &Len);
      Ptr += Len;
      // NumToSkip is a plain 24-bit integer.
      unsigned NumToSkip = *Ptr++;
      NumToSkip |= (*Ptr++) << 8;
      NumToSkip |= (*Ptr++) << 16;

      // Perform the filter operation.
      if (Val != CurFieldValue)
        Ptr += NumToSkip;
      break;
    }
    case MCD::OPC_CheckField: {
      unsigned Start = *++Ptr;
      unsigned Len = *++Ptr;
      uint64_t FieldValue = fieldFromInstruction(insn, Start, Len);
      // Decode the field value.
      unsigned PtrLen = 0;
      uint64_t ExpectedValue = decodeULEB128(++Ptr, &PtrLen);
      Ptr += PtrLen;
      // NumToSkip is a plain 24-bit integer.
      unsigned NumToSkip = *Ptr++;
      NumToSkip |= (*Ptr++) << 8;
      NumToSkip |= (*Ptr++) << 16;

      // If the actual and expected values don't match, skip.
      if (ExpectedValue != FieldValue)
        Ptr += NumToSkip;
      break;
    }
    case MCD::OPC_CheckPredicate: {
      unsigned Len;
      // Decode the Predicate Index value.
      unsigned PIdx = decodeULEB128(++Ptr, &Len);
      Ptr += Len;
      // NumToSkip is a plain 24-bit integer.
      unsigned NumToSkip = *Ptr++;
      NumToSkip |= (*Ptr++) << 8;
      NumToSkip |= (*Ptr++) << 16;
      // Check the predicate.
      bool Pred;
      if (!(Pred = checkDecoderPredicate(PIdx, Bits)))
        Ptr += NumToSkip;
      break;
    }
    case MCD::OPC_Decode: {
      unsigned Len;
      // Decode the Opcode value.
      unsigned Opc = decodeULEB128(++Ptr, &Len);
      Ptr += Len;
      DecodeIdx = decodeULEB128(Ptr, &Len);
      Ptr += Len;

      //   MI.clear();
      MI.setOpcode(Opc);
      // bool DecodeComplete = true;
      // S = decodeToMCInst(S, DecodeIdx, insn, MI, Address, DisAsm,
      //                    DecodeComplete);
      return S;
    }
    case MCD::OPC_TryDecode: {
      unsigned Len;
      // Decode the Opcode value.
      unsigned Opc = decodeULEB128(++Ptr, &Len);
      Ptr += Len;
      DecodeIdx = decodeULEB128(Ptr, &Len);
      Ptr += Len;
      // NumToSkip is a plain 24-bit integer.
      unsigned NumToSkip = *Ptr++;
      NumToSkip |= (*Ptr++) << 8;
      NumToSkip |= (*Ptr++) << 16;

      // Perform the decode operation.
      MCInstGPU TmpMI;
      TmpMI.setOpcode(Opc);
      bool DecodeComplete = true;
      // S = decodeToMCInst(S, DecodeIdx, insn, TmpMI, Address, DisAsm,
      //                    DecodeComplete);
      if (DecodeComplete) {
        // Decoding complete.
        MI = TmpMI;
        return S;
      } else {
        // assert(S == MCDisassembler::Fail);
        // If the decoding was incomplete, skip.
        Ptr += NumToSkip;
        // Reset decode status. This also drops a SoftFail status that could be
        // set before the decode attempt.
        S = MCDisassembler::Success;
      }
      break;
    }
    case MCD::OPC_SoftFail: {
      // Decode the mask values.
      unsigned Len;
      uint64_t PositiveMask = decodeULEB128(++Ptr, &Len);
      Ptr += Len;
      uint64_t NegativeMask = decodeULEB128(Ptr, &Len);
      Ptr += Len;
      bool Fail = (insn & PositiveMask) != 0 || (~insn & NegativeMask) != 0;
      if (Fail)
        S = MCDisassembler::SoftFail;
      break;
    }
    case MCD::OPC_Fail: {
      return MCDisassembler::Fail;
    }
    }
  }
}