file(GLOB children *)
set(DISASSEMBLER_SOURCE)
foreach(child ${children})
    if(IS_DIRECTORY ${child})
        message(STATUS "Adding subdirectory: ${child}")
        LIST(APPEND DISASSEMBLER_SOURCE ${child}/Disassembler.cpp)
    endif()
endforeach()

add_library(
    SyclDisassembler
    ${CMAKE_CURRENT_SOURCE_DIR}/Disassemblers.cpp
    ${DISASSEMBLER_SOURCE}
)

message("CXX_FLAGS: ${CXX_FLAGS}")

target_compile_options(SyclDisassembler PRIVATE -fsycl -fsycl-unnamed-lambda -ferror-limit=1 -Wall -Wpedantic ${CXX_FLAGS})

target_link_libraries(SyclDisassembler PRIVATE -fsycl ${LLVM_LIBS} LIEF::LIEF Boost::program_options)

# message("Using llvm binary dir ${LLVM_BINARY_DIR}")

# target_include_directories(SyclDisassembler PUBLIC ${LLVM_BINARY_DIR}/include)

