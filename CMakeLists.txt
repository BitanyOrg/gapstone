cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

include(FetchContent)
set(FETCHCONTENT_QUIET off)

FetchContent_Declare(LLVM
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llvm-project/llvm
  EXCLUDE_FROM_ALL
)

if(${CMAKE_VERSION} VERSION_LESS "3.14.0")
  # CMake 3.11 to 3.13 needs more verbose method to make LIEF available
  FetchContent_GetProperties(LLVM)
  if(NOT LLVM_POPULATED)
    FetchContent_Populate(LLVM)
    add_subdirectory(${LLVM_SOURCE_DIR} ${LLVM_BINARY_DIR})
  endif()
else()
  # CMake 3.14+ has single function to make LIEF available (recommended)
  FetchContent_MakeAvailable(LLVM)
endif()

FetchContent_Declare(LIEF
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/LIEF
  EXCLUDE_FROM_ALL
)


set(LIEF_EXAMPLES OFF)
if(${CMAKE_VERSION} VERSION_LESS "3.14.0")
  # CMake 3.11 to 3.13 needs more verbose method to make LIEF available
  FetchContent_GetProperties(LIEF)
  if(NOT LIEF_POPULATED)
    FetchContent_Populate(LIEF)
    add_subdirectory(${LIEF_SOURCE_DIR} ${LIEF_BINARY_DIR})
  endif()
else()
  # CMake 3.14+ has single function to make LIEF available (recommended)
  FetchContent_MakeAvailable(LIEF)
endif()


set(BOOST_INCLUDE_LIBRARIES graph filesystem system program_options json)
set(BOOST_ENABLE_CMAKE ON)

find_package(Boost 1.83.0 QUIET COMPONENTS ${BOOST_INCLUDE_LIBRARIES})
if(NOT Boost_FOUND)
  message(STATUS "Boost not found, using FetchContent to download it")
  FetchContent_Declare(
    Boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_PROGRESS TRUE
    GIT_TAG boost-1.83.0
  )
  FetchContent_MakeAvailable(Boost)
else()
  message(STATUS "Reusing system Boost")
endif()

add_subdirectory(sycl)