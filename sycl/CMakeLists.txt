
# cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

set(TOOL_NAME gapstone-sycl)
# Try to detect the right SYCL compiler if one is not explicitly specified:
if (NOT CMAKE_CXX_COMPILER)
    if (WIN32)
        set(CMAKE_CXX_COMPILER icx)
    else()
        find_program(HAS_ICPX "icpx" NO_CACHE)
        if (HAS_ICPX)
            set(CMAKE_CXX_COMPILER icpx)
        else()
            set(CMAKE_CXX_COMPILER clang++)
        endif()
    endif()
endif()


# set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE PATH "Build Type" FORCE)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(NODPL    "Disable samples that require the oneAPI DPC++ Library (oneDPL).")
option(NODPCT   "Disable samples that require the DPC++ Compatibility Tool (dpct).")
option(NOL0     "Disable samples that require the oneAPI Level Zero Headers and Loader." ON)
option(WITHCUDA "Enable CUDA device support for the samples.")
option(WITHROCM "Enable ROCm device support for the samples.")

if (WITHCUDA AND WITHROCM)
    message(FATAL_ERROR "WITHCUDA and WITHROCM cannot be enabled at the same time.\n" 
    "Clean up the directory and try again with only one of them enabled.")
endif()

set(CUDA_GPU_ARCH "sm_60" CACHE STRING "CUDA GPUs to compile for.")
if (WITHCUDA)
    mark_as_advanced(CLEAR FORCE CUDA_GPU_ARCH)
else()
    mark_as_advanced(FORCE CUDA_GPU_ARCH)
endif()

set(ROCM_GPU_ARCH "gfx1100" CACHE STRING "ROCm GPUs to compile for.")
if (WITHROCM)
    mark_as_advanced(CLEAR FORCE ROCM_GPU_ARCH)
else()
    mark_as_advanced(FORCE ROCM_GPU_ARCH)
endif()

list(APPEND CMAKE_MODULE_PATH "${LLVM_BINARY_DIR}/cmake/modules/CMakeFiles")
include(LLVMConfig)

set(TABLEGEN_INCLUDE_DIRS)
FOREACH(t ${LLVM_TARGETS_TO_BUILD})
    LIST(APPEND TABLEGEN_INCLUDE_DIRS "${LLVM_BINARY_DIR}/lib/Target/${t}")
    LIST(APPEND TABLEGEN_INCLUDE_DIRS "${LLVM_SOURCE_DIR}/lib/Target/${t}")
ENDFOREACH(t)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/tblgen ${LLVM_BINARY_DIR}/include ${TABLEGEN_INCLUDE_DIRS} ${LLVM_SOURCE_DIR}/lib/Target ${LLVM_SOURCE_DIR}/include)

set(LLVM_LINK_COMPONENTS
    Core
    Support
    TargetParser
    MC
    MCDisassembler
    AllTargetsCodeGens
    AllTargetsAsmParsers
    AllTargetsDescs
    AllTargetsDisassemblers
    AllTargetsInfos
    AllTargetsMCAs
  )

llvm_map_components_to_libnames(LLVM_LIBS ${LLVM_LINK_COMPONENTS})

# Prepend LLVM in front of every target, this is how the library
# are named with CMake
# FOREACH(t ${TARGET_LIBS})
# LIST(APPEND LLVM_LIBS "LLVM${t}")
# ENDFOREACH(t)

# set(LLVM_LIBS LLVMSupport LLVMTargetParser LLVMMC)

set (CXX_FLAGS ${CXX_FLAGS} -Wno-language-extension-token -Wno-return-type -Wno-unused-function)

if(WITHCUDA)
    set(CXX_FLAGS ${CXX_FLAGS} -fsycl-targets=nvptx64-nvidia-cuda,spir64 -Xsycl-target-backend=nvptx64-nvidia-cuda --cuda-gpu-arch=${CUDA_GPU_ARCH})
    set(LIBS ${LIBS} -fsycl-targets=nvptx64-nvidia-cuda,spir64 -Xsycl-target-backend=nvptx64-nvidia-cuda --cuda-gpu-arch=${CUDA_GPU_ARCH})
endif()

if(WITHROCM)
    set(CXX_FLAGS ${CXX_FLAGS} -fsycl-targets=amdgcn-amd-amdhsa,spir64 -Xsycl-target-backend=amdgcn-amd-amdhsa --offload-arch=${ROCM_GPU_ARCH})
    set(LIBS ${LIBS} -fsycl-targets=amdgcn-amd-amdhsa,spir64 -Xsycl-target-backend=amdgcn-amd-amdhsa --offload-arch=${ROCM_GPU_ARCH})
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(
    ${TOOL_NAME}
    src/main.cpp
)

# target_include_directories(${TOOL_NAME} PUBLIC ${LLVM_SOURCE_DIR}/include)

target_compile_options(${TOOL_NAME} PRIVATE -fsycl -fsycl-unnamed-lambda -ferror-limit=1 -Wall -Wpedantic ${CXX_FLAGS})

target_link_libraries(${TOOL_NAME} PRIVATE ${LIBS} -fsycl ${LLVM_LIBS} LIEF::LIEF Boost::program_options SyclDisassembler)

if(CMAKE_CONFIGURATION_TYPES)
    set(CONFIGS ${CMAKE_CONFIGURATION_TYPES})
else()
    set(CONFIGS ${CMAKE_BUILD_TYPE})
endif()
foreach(CONFIG ${CONFIGS})
    install(TARGETS ${TOOL_NAME} CONFIGURATIONS ${CONFIG} DESTINATION ${CONFIG})
endforeach()
if(TEST)
    add_test(NAME ${TOOL_NAME} COMMAND ${TOOL_NAME})
endif()


if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install" CACHE PATH "Install Path" FORCE)
endif()