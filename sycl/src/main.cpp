// Copyright (C) 2023 Intel Corporation

// SPDX-License-Identifier: MIT

#include "Disassemblers.h"
#include "MCInstGPU.h"
#include "SyclDisassembler.h"
#include <LIEF/LIEF.hpp>
#include <access/access.hpp>
#include <boost/program_options.hpp>
#include <device_selector.hpp>
#include <exception.hpp>
#include <iostream>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCDecoderOps.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/MCInstrDesc.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/MC/MCTargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Triple.h>
#include <sycl/sycl.hpp>

using namespace sycl;
using namespace llvm;
using DecodeStatus = llvm::MCDisassembler::DecodeStatus;

namespace po = boost::program_options;

struct Args {
  std::string file_path;
  std::optional<std::string> triple;
  std::string cpu;
  std::string features;
  int step_size;
  bool naive;
};

std::optional<Args> ParseArgs(int argc, char *argvp[]) {
  po::options_description desc("Tool to extract edges from binary");
  desc.add_options()("file_path,f", po::value<std::string>(),
                     "File to disassemble")(
      "triple,t", po::value<std::string>(),
      "Target triple")("cpu,c", po::value<std::string>(), "CPU")(
      "features,r", po::value<std::string>(),
      "Features")("step_size,s", po::value<int>(), "Step Size")(
      "naive,n", "Use naive implementation")("help,h",
                                                                "Print help");
  po::positional_options_description p;
  p.add("file_path", 1);

  po::variables_map vm;
  po::store(
      po::command_line_parser(argc, argvp).options(desc).positional(p).run(),
      vm);
  po::notify(vm);
  if (vm.count("help") || argc == 1) {
    std::cout << "Usage: " << argvp[0] << " [options] <file_path>" << std::endl;
    std::cout << desc << std::endl;
    return std::nullopt;
  }
  return std::make_optional<Args>(Args{
      vm["file_path"].as<std::string>(),
      vm.count("triple") ? std::make_optional(vm["triple"].as<std::string>())
                         : std::nullopt,
      vm.count("cpu") ? vm["cpu"].as<std::string>() : "",
      vm.count("features") ? vm["features"].as<std::string>() : "",
      vm.count("step_size") ? vm["step_size"].as<int>() : 1,
      vm.count("naive") ? true : false,
  });
}

std::optional<llvm::MCInst>
disassemble(std::unique_ptr<llvm::MCDisassembler> &disassembler,
            const llvm::ArrayRef<uint8_t> &data, uint64_t address) {
  // Disassemble
  // Decompose one instruction
  llvm::MCInst insn;
  uint64_t insn_size = 0;
  auto res = disassembler->getInstruction(insn, insn_size, data, address,
                                          llvm::nulls());
  if (res == llvm::MCDisassembler::Fail ||
      res == llvm::MCDisassembler::SoftFail) {
    std::stringstream error_stream;
    error_stream << "Could not disassemble at address " << std::hex << address;
    return std::nullopt;
  }
  return insn;
}

int main(int argc, char **argv) {
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllDisassemblers();
  // Create queue on whatever default device that the
  // implementation chooses. Implicit use of
  // default_selector_v
  queue q;
  // queue q;
  auto args = ParseArgs(argc, argv);
  if (!args) {
    return -1;
  }
  auto elf = LIEF::Parser::parse(args->file_path);
  auto triple = llvm::Triple(llvm::Triple::normalize(*args->triple));
  std::string lookup_target_error;
  const llvm::Target *target = llvm::TargetRegistry::lookupTarget(
      triple.getTriple(), lookup_target_error);
  if (target == nullptr) {
    // return tl::unexpected(lookup_target_error);
    return -1;
  }

  // Init reusable llvm info objects
  auto register_info = std::unique_ptr<llvm::MCRegisterInfo>(
      target->createMCRegInfo(triple.getTriple()));
  if (!register_info) {
    return -1;
  }

  llvm::MCTargetOptions target_options;
  auto assembler_info =
      std::unique_ptr<llvm::MCAsmInfo>(target->createMCAsmInfo(
          *register_info, triple.getTriple(), target_options));
  if (!assembler_info) {
    return -1;
  }

  auto instruction_info =
      std::unique_ptr<llvm::MCInstrInfo>(target->createMCInstrInfo());
  if (!instruction_info) {
    return -1;
  }

  auto subtarget_info =
      std::unique_ptr<llvm::MCSubtargetInfo>(target->createMCSubtargetInfo(
          triple.getTriple(), args->cpu, args->features));
  if (!subtarget_info) {
    return -1;
  }

  llvm::MCContext context(triple, assembler_info.get(), register_info.get(),
                          subtarget_info.get(), nullptr, &target_options);

  // Create instruction printer
  // For x86 and x86_64 switch to intel assembler dialect
  auto syntax_variant = assembler_info->getAssemblerDialect();
  if (triple.getArch() == llvm::Triple::x86 ||
      triple.getArch() == llvm::Triple::x86_64) {
    syntax_variant = 1;
  }
  auto instruction_printer = std::unique_ptr<llvm::MCInstPrinter>(
      target->createMCInstPrinter(triple, syntax_variant, *assembler_info,
                                  *instruction_info, *register_info));
  if (!instruction_printer) {
    return -1;
  }

  // Create disassembler
  auto disassembler = std::unique_ptr<llvm::MCDisassembler>(
      target->createMCDisassembler(*subtarget_info, context));
  if (!disassembler) {
    return -1;
  }

  auto gapstone_disassembler = gapstone::createDisassembler(*disassembler, q);
  std::cout << "Processing Arch " << to_string(elf->header().architecture())
            << std::endl;
  for (auto &section : elf->sections()) {
    if (section.name() != ".text") {
      continue;
    }
    auto base_addr = section.virtual_address();
    uint64_t len = section.content().size();
    auto content = elf->get_content_from_virtual_address(base_addr, len);
    if (args->naive) {

      for (int i = 0; i < len; i += args->step_size) {
        auto offset = i * args->step_size;
        const llvm::ArrayRef<uint8_t> data(content.begin() + offset,
                                           content.end());
        auto insn = disassemble(disassembler, data, base_addr + offset);
        if (!insn) {
          continue;
        }
        std::string insn_str;
        llvm::raw_string_ostream str_stream(insn_str);
        if (insn) {
          instruction_printer->printInst(
              &*insn,
              /* Address */ base_addr + args->step_size * i,
              /* Annot */ "", *subtarget_info, str_stream);
          std::cout << "0x" << std::hex << base_addr + offset << " " << insn_str
                    << std::endl;
        }
      }
    } else {

      std::vector<uint8_t> content_vector{content.begin(), content.end()};
      auto insns = gapstone_disassembler->batch_disassemble(
          base_addr, content_vector, args->step_size);
      for (int i = 0; i < insns.size(); ++i) {
        if (insns[i].status != llvm::MCDisassembler::DecodeStatus::Success) {
          continue;
        }
        std::cout << "0x" << std::hex << base_addr + args->step_size * i << " "
                  << std::flush;
        std::string insn_str;
        llvm::raw_string_ostream str_stream(insn_str);
        instruction_printer->printInst(
            &insns[i].inst,
            /* Address */ base_addr + args->step_size * i,
            /* Annot */ "", *subtarget_info, str_stream);
        std::cout << insn_str << std::endl;
      }
    }
  }

  std::cout << "Selected device: "
            << q.get_device().get_info<info::device::name>() << "\n";

  return 0;
}
