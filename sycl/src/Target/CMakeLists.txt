file(GLOB children *)
set(DISASSEMBLER_SOURCE)

# Define the list of excluded directories
set(EXCLUDED_DIRS "ARM")

foreach(child ${children})
    if(IS_DIRECTORY ${child})
        # Extract the last name of the directory
        get_filename_component(dirname ${child} NAME)

        # Check if the last name is in the list of excluded directories
        list(FIND EXCLUDED_DIRS ${dirname} index)
        if(${index} EQUAL -1)
            message(STATUS "Adding subdirectory: ${child} (last name: ${dirname})")
            list(APPEND DISASSEMBLER_SOURCE ${child}/Disassembler.cpp)
        else()
            message(STATUS "Excluding directory: ${child} (last name: ${dirname})")
        endif()
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

