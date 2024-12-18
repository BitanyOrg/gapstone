#include "ARM/ARMSyclDisassembler.h"
#include "ARM/Decode.h"
#include "DecodeInstruction.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/bit.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include <range.hpp>
#include <sycl/sycl.hpp>
#include <usm.hpp>
#include <vector>
namespace gapstone {

namespace ARMImpl {

DecodeStatus getThumbInstruction(MCInst &MI, uint64_t &Size,
                                 ArrayRef<uint8_t> Bytes, uint64_t Address,
                                 const FeatureBitset &Bits) {

  // We want to read exactly 2 bytes of data.
  llvm::endianness InstructionEndianness = Bits[ARM::ModeBigEndianInstructions]
                                               ? llvm::endianness::big
                                               : llvm::endianness::little;
  if (Bytes.size() < 2) {
    Size = 0;
    return MCDisassembler::Fail;
  }

  uint16_t Insn16 = llvm::support::endian::read<uint16_t>(
      Bytes.data(), InstructionEndianness);
  DecodeStatus Result = decodeInstruction(DecoderTableThumb16, MI, Insn16,
                                          Address, nullptr, Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 2;
    // Check(Result, AddThumbPredicate(MI));
    return Result;
  }

  Result = decodeInstruction(DecoderTableThumbSBit16, MI, Insn16, Address,
                             nullptr, Bits);
  if (Result) {
    Size = 2;
    // bool InITBlock = ITBlock.instrInITBlock();
    // Check(Result, AddThumbPredicate(MI));
    // AddThumb1SBit(MI, InITBlock);
    return Result;
  }

  Result = decodeInstruction(DecoderTableThumb216, MI, Insn16, Address, nullptr,
                             Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 2;

    // Nested IT blocks are UNPREDICTABLE.  Must be checked before we add
    // the Thumb predicate.
    // if (MI.getOpcode() == ARM::t2IT && ITBlock.instrInITBlock())
    //   Result = MCDisassembler::SoftFail;

    // Check(Result, AddThumbPredicate(MI));

    // If we find an IT instruction, we need to parse its condition
    // code and mask operands so that we can apply them correctly
    // to the subsequent instructions.
    // if (MI.getOpcode() == ARM::t2IT) {
    //   unsigned Firstcond = MI.getOperand(0).getImm();
    //   unsigned Mask = MI.getOperand(1).getImm();
    //   ITBlock.setITState(Firstcond, Mask);

    //   // An IT instruction that would give a 'NV' predicate is unpredictable.
    //   if (Firstcond == ARMCC::AL && !isPowerOf2_32(Mask))
    // }

    return Result;
  }

  // We want to read exactly 4 bytes of data.
  if (Bytes.size() < 4) {
    Size = 0;
    return MCDisassembler::Fail;
  }

  uint32_t Insn32 =
      (uint32_t(Insn16) << 16) | llvm::support::endian::read<uint16_t>(
                                     Bytes.data() + 2, InstructionEndianness);

  Result =
      decodeInstruction(DecoderTableMVE32, MI, Insn32, Address, nullptr, Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 4;

    // Nested VPT blocks are UNPREDICTABLE. Must be checked before we add
    // the VPT predicate.
    // if (isVPTOpcode(MI.getOpcode()) && VPTBlock.instrInVPTBlock())
    //   Result = MCDisassembler::SoftFail;

    // Check(Result, AddThumbPredicate(MI));

    // if (isVPTOpcode(MI.getOpcode())) {
    //   unsigned Mask = MI.getOperand(0).getImm();
    //   VPTBlock.setVPTState(Mask);
    // }

    return Result;
  }

  Result = decodeInstruction(DecoderTableThumb32, MI, Insn32, Address, nullptr,
                             Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    // bool InITBlock = ITBlock.instrInITBlock();
    // Check(Result, AddThumbPredicate(MI));
    // AddThumb1SBit(MI, InITBlock);
    return Result;
  }

  Result = decodeInstruction(DecoderTableThumb232, MI, Insn32, Address, nullptr,
                             Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    // Check(Result, AddThumbPredicate(MI));
    return checkDecodedInstruction(MI, Size, Address, Insn32, Result);
  }

  if (fieldFromInstruction(Insn32, 28, 4) == 0xE) {
    Result = decodeInstruction(DecoderTableVFP32, MI, Insn32, Address, nullptr,
                               Bits);
    if (Result != MCDisassembler::Fail) {
      Size = 4;
      // UpdateThumbVFPPredicate(Result, MI);
      return Result;
    }
  }

  Result = decodeInstruction(DecoderTableVFPV832, MI, Insn32, Address, nullptr,
                             Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return Result;
  }

  if (fieldFromInstruction(Insn32, 28, 4) == 0xE) {
    Result = decodeInstruction(DecoderTableNEONDup32, MI, Insn32, Address,
                               nullptr, Bits);
    if (Result != MCDisassembler::Fail) {
      Size = 4;
      // Check(Result, AddThumbPredicate(MI));
      return Result;
    }
  }

  if (fieldFromInstruction(Insn32, 24, 8) == 0xF9) {
    uint32_t NEONLdStInsn = Insn32;
    NEONLdStInsn &= 0xF0FFFFFF;
    NEONLdStInsn |= 0x04000000;
    Result = decodeInstruction(DecoderTableNEONLoadStore32, MI, NEONLdStInsn,
                               Address, nullptr, Bits);
    if (Result != MCDisassembler::Fail) {
      Size = 4;
      // Check(Result, AddThumbPredicate(MI));
      return Result;
    }
  }

  if (fieldFromInstruction(Insn32, 24, 4) == 0xF) {
    uint32_t NEONDataInsn = Insn32;
    NEONDataInsn &= 0xF0FFFFFF;                       // Clear bits 27-24
    NEONDataInsn |= (NEONDataInsn & 0x10000000) >> 4; // Move bit 28 to bit 24
    NEONDataInsn |= 0x12000000;                       // Set bits 28 and 25
    Result = decodeInstruction(DecoderTableNEONData32, MI, NEONDataInsn,
                               Address, nullptr, Bits);
    if (Result != MCDisassembler::Fail) {
      Size = 4;
      // Check(Result, AddThumbPredicate(MI));
      return Result;
    }

    uint32_t NEONCryptoInsn = Insn32;
    NEONCryptoInsn &= 0xF0FFFFFF; // Clear bits 27-24
    NEONCryptoInsn |=
        (NEONCryptoInsn & 0x10000000) >> 4; // Move bit 28 to bit 24
    NEONCryptoInsn |= 0x12000000;           // Set bits 28 and 25
    Result = decodeInstruction(DecoderTablev8Crypto32, MI, NEONCryptoInsn,
                               Address, nullptr, Bits);
    if (Result != MCDisassembler::Fail) {
      Size = 4;
      return Result;
    }

    uint32_t NEONv8Insn = Insn32;
    NEONv8Insn &= 0xF3FFFFFF; // Clear bits 27-26
    Result = decodeInstruction(DecoderTablev8NEON32, MI, NEONv8Insn, Address,
                               nullptr, Bits);
    if (Result != MCDisassembler::Fail) {
      Size = 4;
      return Result;
    }
  }

  uint32_t Coproc = fieldFromInstruction(Insn32, 8, 4);
  const uint8_t *DecoderTable =
      ((Coproc < 8) & Bits[ARM::FeatureCoprocCDE0 + Coproc])
          ? DecoderTableThumb2CDE32
          : DecoderTableThumb2CoProc32;
  Result = decodeInstruction(DecoderTable, MI, Insn32, Address, nullptr, Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    // Check(Result, AddThumbPredicate(MI));
    return Result;
  }

  Size = 0;
  return MCDisassembler::Fail;
}

DecodeStatus getARMInstruction(MCInst &MI, uint64_t &Size,
                               ArrayRef<uint8_t> Bytes, uint64_t Address,
                               const FeatureBitset &Bits) {
  llvm::endianness InstructionEndianness = Bits[ARM::ModeBigEndianInstructions]
                                               ? llvm::endianness::big
                                               : llvm::endianness::little;
  // We want to read exactly 4 bytes of data.
  if (Bytes.size() < 4) {
    Size = 0;
    return MCDisassembler::Fail;
  }

  // Encoded as a 32-bit word in the stream.
  uint32_t Insn = llvm::support::endian::read<uint32_t>(Bytes.data(),
                                                        InstructionEndianness);

  // Calling the auto-generated decoder function.
  DecodeStatus Result =
      decodeInstruction(DecoderTableARM32, MI, Insn, Address, nullptr, Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return checkDecodedInstruction(MI, Size, Address, Insn, Result);
  }

  struct DecodeTable {
    const uint8_t *P;
    bool DecodePred;
  };

  const DecodeTable Tables[] = {
      {DecoderTableVFP32, false},      {DecoderTableVFPV832, false},
      {DecoderTableNEONData32, true},  {DecoderTableNEONLoadStore32, true},
      {DecoderTableNEONDup32, true},   {DecoderTablev8NEON32, false},
      {DecoderTablev8Crypto32, false},
  };

  for (auto Table : Tables) {
    Result = decodeInstruction(Table.P, MI, Insn, Address, nullptr, Bits);
    if (Result != MCDisassembler::Fail) {
      Size = 4;
      // Add a fake predicate operand, because we share these instruction
      // definitions with Thumb2 where these instructions are predicable.
      // if (Table.DecodePred &&
      //     !DecodePredicateOperand(MI, 0xE, Address, nullptr))
      //   return MCDisassembler::Fail;
      return Result;
    }
  }

  Result =
      decodeInstruction(DecoderTableCoProc32, MI, Insn, Address, nullptr, Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return checkDecodedInstruction(MI, Size, Address, Insn, Result);
  }

  Size = 4;
  return MCDisassembler::Fail;
}

static DecodeStatus disassemble_instruction(MCInst &MI, uint64_t &Size,
                                            ArrayRef<uint8_t> Bytes,
                                            uint64_t Address,
                                            const FeatureBitset &Bits) {
  if (Bits[ARM::ModeThumb])
    return getThumbInstruction(MI, Size, Bytes, Address, Bits);
  return getARMInstruction(MI, Size, Bytes, Address, Bits);
}

#include "DisassembleImpl.h"
} // namespace ARMImpl

std::unique_ptr<InstInfoContainer> ARMDisassembler::batch_disassemble(
    uint64_t base_addr, std::vector<uint8_t> &content, int step_size) {
  return ARMImpl::disassemble_impl<MCInstGPU_ARM>(q, MCDisassembler, base_addr,
                                                  content, step_size);
}
} // namespace gapstone