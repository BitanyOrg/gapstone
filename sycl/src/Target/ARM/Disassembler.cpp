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

const uint8_t *const ThumbDecodeTables[] = {
    DecoderTableThumb16,        DecoderTableThumbSBit16,
    DecoderTableThumb216,       DecoderTableMVE32,
    DecoderTableThumb32,        DecoderTableThumb232,
    DecoderTableVFP32,          DecoderTableVFPV832,
    DecoderTableNEONDup32,      DecoderTableNEONLoadStore32,
    DecoderTableNEONData32,     DecoderTablev8Crypto32,
    DecoderTablev8NEON32,       DecoderTableThumb2CDE32,
    DecoderTableThumb2CoProc32,
};

const uint8_t *const ARMDecodeTables[] = {
    DecoderTableARM32,      DecoderTableVFP32,           DecoderTableVFPV832,
    DecoderTableNEONData32, DecoderTableNEONLoadStore32, DecoderTableNEONDup32,
    DecoderTablev8NEON32,   DecoderTablev8Crypto32,      DecoderTableCoProc32};

template <typename T>
DecodeStatus getThumbInstruction(T &MI, unsigned &Size, ArrayRef<uint8_t> Bytes,
                                 uint64_t Address, const FeatureBitset &Bits) {

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
  DecodeStatus Result =
      decodeOpCode(DecoderTableThumb16, MI, Insn16, Address, nullptr, Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 2;
    return Result;
  }

  Result =
      decodeOpCode(DecoderTableThumbSBit16, MI, Insn16, Address, nullptr, Bits);
  if (Result) {
    Size = 2;
    return Result;
  }

  Result =
      decodeOpCode(DecoderTableThumb216, MI, Insn16, Address, nullptr, Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 2;
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

  Result = decodeOpCode(DecoderTableMVE32, MI, Insn32, Address, nullptr, Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return Result;
  }

  Result =
      decodeOpCode(DecoderTableThumb32, MI, Insn32, Address, nullptr, Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return Result;
  }

  Result =
      decodeOpCode(DecoderTableThumb232, MI, Insn32, Address, nullptr, Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return Result;
  }

  if (fieldFromInstruction(Insn32, 28, 4) == 0xE) {
    Result =
        decodeOpCode(DecoderTableVFP32, MI, Insn32, Address, nullptr, Bits);
    if (Result != MCDisassembler::Fail) {
      Size = 4;
      return Result;
    }
  }

  Result =
      decodeOpCode(DecoderTableVFPV832, MI, Insn32, Address, nullptr, Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return Result;
  }

  if (fieldFromInstruction(Insn32, 28, 4) == 0xE) {
    Result =
        decodeOpCode(DecoderTableNEONDup32, MI, Insn32, Address, nullptr, Bits);
    if (Result != MCDisassembler::Fail) {
      Size = 4;
      return Result;
    }
  }

  if (fieldFromInstruction(Insn32, 24, 8) == 0xF9) {
    uint32_t NEONLdStInsn = Insn32;
    NEONLdStInsn &= 0xF0FFFFFF;
    NEONLdStInsn |= 0x04000000;
    Result = decodeOpCode(DecoderTableNEONLoadStore32, MI, NEONLdStInsn,
                          Address, nullptr, Bits);
    if (Result != MCDisassembler::Fail) {
      Size = 4;
      return Result;
    }
  }

  if (fieldFromInstruction(Insn32, 24, 4) == 0xF) {
    uint32_t NEONDataInsn = Insn32;
    NEONDataInsn &= 0xF0FFFFFF;                       // Clear bits 27-24
    NEONDataInsn |= (NEONDataInsn & 0x10000000) >> 4; // Move bit 28 to bit 24
    NEONDataInsn |= 0x12000000;                       // Set bits 28 and 25
    Result = decodeOpCode(DecoderTableNEONData32, MI, NEONDataInsn, Address,
                          nullptr, Bits);
    if (Result != MCDisassembler::Fail) {
      Size = 4;
      return Result;
    }

    uint32_t NEONCryptoInsn = Insn32;
    NEONCryptoInsn &= 0xF0FFFFFF; // Clear bits 27-24
    NEONCryptoInsn |=
        (NEONCryptoInsn & 0x10000000) >> 4; // Move bit 28 to bit 24
    NEONCryptoInsn |= 0x12000000;           // Set bits 28 and 25
    Result = decodeOpCode(DecoderTablev8Crypto32, MI, NEONCryptoInsn, Address,
                          nullptr, Bits);
    if (Result != MCDisassembler::Fail) {
      Size = 4;
      return Result;
    }

    uint32_t NEONv8Insn = Insn32;
    NEONv8Insn &= 0xF3FFFFFF; // Clear bits 27-26
    Result = decodeOpCode(DecoderTablev8NEON32, MI, NEONv8Insn, Address,
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
  Result = decodeOpCode(DecoderTable, MI, Insn32, Address, nullptr, Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return Result;
  }

  Size = 0;
  return MCDisassembler::Fail;
}

template <typename T>
DecodeStatus getThumbOperands(T &MI, uint64_t Address,
                              const FeatureBitset &Bits) {}

template <typename T>
DecodeStatus getARMInstruction(T &MI, unsigned &Size, ArrayRef<uint8_t> Bytes,
                               uint64_t Address, const FeatureBitset &Bits) {
  llvm::endianness InstructionEndianness = Bits[ARM::ModeBigEndianInstructions]
                                               ? llvm::endianness::big
                                               : llvm::endianness::little;
  // We want to read exactly 4 bytes of data.
  if (Bytes.size() < 4) {
    Size = 0;
    return MCDisassembler::Fail;
  }

  // Encoded as a 32-bit word in the stream.
  uint32_t Insn32 = llvm::support::endian::read<uint32_t>(
      Bytes.data(), InstructionEndianness);

  // Calling the auto-generated decoder function.
  DecodeStatus Result =
      decodeOpCode(DecoderTableARM32, MI, Insn32, Address, nullptr, Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    // return checkDecodedInstruction(MI, Size, Address, Insn32, Result);
    return Result;
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
    Result = decodeOpCode(Table.P, MI, Insn32, Address, nullptr, Bits);
    if (Result != MCDisassembler::Fail) {
      Size = 4;
      return Result;
    }
  }

  Result =
      decodeOpCode(DecoderTableCoProc32, MI, Insn32, Address, nullptr, Bits);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return Result;
  }

  Size = 4;
  return MCDisassembler::Fail;
}

template <typename T>
DecodeStatus getARMOperands(T &MI, uint64_t Address,
                            const FeatureBitset &Bits) {}

static DecodeStatus decode_instruction(MCInst &MI, ArrayRef<uint8_t> Bytes,
                                       uint64_t Address,
                                       const FeatureBitset &Bits) {
  if (Bits[ARM::ModeThumb])
    return getThumbInstruction(MI, MI.Size, Bytes, Address, Bits);
  return getARMInstruction(MI, MI.Size, Bytes, Address, Bits);
}

#include "DisassembleImpl.h"
} // namespace ARMImpl

std::unique_ptr<InstInfoContainer> ARMDisassembler::batch_disassemble(
    uint64_t base_addr, std::vector<uint8_t> &content, int step_size) {
  return ARMImpl::decode_impl<MCInstGPU_ARM>(q, MCDisassembler, base_addr,
                                             content, step_size);
}
} // namespace gapstone