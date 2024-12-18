#include "SyclDisassembler.h"
#include <memory>
#include <sycl/sycl.hpp>
// template<typename T>
// static std::unique_ptr<gapstone::InstInfoContainer>
// disassemble_impl(sycl::queue &q, llvm::MCDisassembler &MCDisassembler,
//                  uint64_t base_addr, std::vector<uint8_t> &content,
//                  int step_size) {
//   auto tasks = content.size() / step_size;
//   const FeatureBitset &Bits =
//       MCDisassembler.getSubtargetInfo().getFeatureBits();
//   auto buffer_size = content.size();
//   auto res = std::make_unique<InstInfoContainerGPU<T>>(tasks);
//   uint8_t *device_content = sycl::malloc_device<uint8_t>(content.size(), q);
//   auto event_copy = q.memcpy(device_content, content.data(), content.size());
//   sycl::buffer gpu_insts_buffer(res->insts);
//   sycl::buffer status_buffer{res->status};
//   q.submit([&](sycl::handler &h) {
//     sycl::accessor inst_accessor(gpu_insts_buffer, h, sycl::write_only);
//     sycl::accessor status_accessor(status_buffer, h, sycl::write_only);
//     h.depends_on(event_copy);
//     h.parallel_for(tasks, [=](sycl::id<1> i) {
//       uint64_t size;
//       uint64_t offset = i * step_size;
//       llvm::ArrayRef<uint8_t> array_ref(device_content + offset,
//                                         buffer_size - offset);
//       status_accessor[i] = disassemble_instruction(
//           inst_accessor[i], size, array_ref, base_addr + offset, Bits);
//     });
//   });
//   q.wait();
//   return res;
// }

template <typename T>
static std::unique_ptr<gapstone::InstInfoContainer>
disassemble_impl(sycl::queue &q, llvm::MCDisassembler &MCDisassembler,
                 uint64_t base_addr, std::vector<uint8_t> &content,
                 int step_size) {
  auto tasks = content.size() / step_size;
  const FeatureBitset &Bits =
      MCDisassembler.getSubtargetInfo().getFeatureBits();
  auto buffer_size = content.size();
  T *gpu_insts = sycl::malloc_shared<T>(tasks, q);
  DecodeStatus *status = sycl::malloc_shared<DecodeStatus>(tasks, q);
  uint8_t *device_content = sycl::malloc_device<uint8_t>(content.size(), q);
  auto event_copy = q.memcpy(device_content, content.data(), content.size());
  auto event_disassemble = q.submit([&](sycl::handler &h) {
    h.depends_on(event_copy);
    h.parallel_for(tasks, [=](sycl::id<1> i) {
      uint64_t size;
      uint64_t offset = i * step_size;
      llvm::ArrayRef<uint8_t> array_ref(device_content + offset,
                                        buffer_size - offset);
      status[i] = disassemble_instruction(gpu_insts[i], size, array_ref,
                                          base_addr + offset, Bits);
    });
  });
  event_disassemble.wait();
  auto res = std::make_unique<gapstone::InstInfoContainerGPU<T>>(tasks);
  q.memcpy(res->status.data(), status, tasks * sizeof(DecodeStatus));
  q.memcpy(res->insts.data(), gpu_insts, tasks * sizeof(T));
  q.wait();
  sycl::free(status, q);
  sycl::free(gpu_insts, q);
  sycl::free(device_content, q);
  return res;
}
