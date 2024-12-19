#include "MCInstGPU.h"
#include <llvm/MC/MCDecoderOps.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/Support/LEB128.h>
#include <llvm/TargetParser/SubtargetFeature.h>
using DecodeStatus = llvm::MCDisassembler::DecodeStatus;
using namespace llvm;

template <typename InsnType, typename T>
static DecodeStatus
decodeInstruction(const uint8_t DecodeTable[], T &MI, InsnType insn,
                  uint64_t Address, const MCDisassembler *DisAsm,
                  const FeatureBitset &Bits,
                  ArrayRef<uint8_t> &Bytes) {
  const uint8_t *Ptr = DecodeTable;
  uint64_t CurFieldValue = 0;
  DecodeStatus S = MCDisassembler::Success;
  while (true) {
    switch (*Ptr) {
    default:
      return MCDisassembler::Fail;
    case MCD::OPC_ExtractField: {
      // Decode the start value.
      unsigned Start = decodeULEB128AndIncUnsafe(++Ptr);
      unsigned Len = *Ptr++;
      makeUp(insn, Start + Len, Bytes);
      CurFieldValue = fieldFromInstruction(insn, Start, Len);
      break;
    }
    case MCD::OPC_FilterValue: {
      // Decode the field value.
      uint64_t Val = decodeULEB128AndIncUnsafe(++Ptr);
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
      // Decode the start value.
      unsigned Start = decodeULEB128AndIncUnsafe(++Ptr);
      unsigned Len = *Ptr;
      makeUp(insn, Start + Len, Bytes);
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
      // Decode the Predicate Index value.
      unsigned PIdx = decodeULEB128AndIncUnsafe(++Ptr);
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
      // Decode the Opcode value.
      unsigned Opc = decodeULEB128AndIncUnsafe(++Ptr);
      unsigned DecodeIdx = decodeULEB128AndIncUnsafe(Ptr);

      MI.clear();
      MI.setOpcode(Opc);
      bool DecodeComplete;
      unsigned Len = InstrLenTable[Opc];
      makeUp(insn, Len, Bytes);
      S = decodeToMCInst(S, DecodeIdx, insn, MI, Address, DisAsm,
                         DecodeComplete);

      return S;
    }
    case MCD::OPC_TryDecode: {
      // Decode the Opcode value.
      unsigned Opc = decodeULEB128AndIncUnsafe(++Ptr);
      unsigned DecodeIdx = decodeULEB128AndIncUnsafe(Ptr);
      // NumToSkip is a plain 24-bit integer.
      unsigned NumToSkip = *Ptr++;
      NumToSkip |= (*Ptr++) << 8;
      NumToSkip |= (*Ptr++) << 16;

      // Perform the decode operation.
      T TmpMI;
      TmpMI.setOpcode(Opc);
      bool DecodeComplete;
      S = decodeToMCInst(S, DecodeIdx, insn, TmpMI, Address, DisAsm,
                         DecodeComplete);

      if (DecodeComplete) {
        // Decoding complete.
        MI = TmpMI;
        return S;
      } else {
        assert(S == MCDisassembler::Fail);
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
      uint64_t PositiveMask = decodeULEB128AndIncUnsafe(++Ptr);
      uint64_t NegativeMask = decodeULEB128AndIncUnsafe(Ptr);
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
