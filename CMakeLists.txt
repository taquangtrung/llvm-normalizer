#
# Author: Ta Quang Trung
# Date:   July 6th, 2020
#

project(llvm-normalizer)

cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 11)

# Determine the OS type
if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
endif()

# Backward debugging trace is supported only in Linux
if(LINUX)
  set(CMAKE_BUILD_TYPE Debug)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
  set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-rdynamic")
endif()

# Set cmake dirs
set(
  CMAKE_DIR   "${CMAKE_CURRENT_LIST_DIR}/cmake"
  CACHE PATH  "The the path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_DIR})

# Set LLVM Version
set(LLVM_VERSION 8)

# config llvm
find_package(LLVM ${LLVM_VERSION} REQUIRED CONFIG)

if (NOT LLVM_DIR)
  message (FATAL_ERROR "Could not find LLVM CMake files")
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "CMake files are located at: ${LLVM_DIR}")
message(STATUS "Using the file: ${LLVM_DIR}/LLVMConfig.cmake")

# The below LLVM variables are read from ${LLVM_DIR}/LLVMConfig.cmake
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Setting for third-party libraries
set(EXTRA_LIB_DIRS "${CMAKE_CURRENT_LIST_DIR}/lib")
include_directories(${EXTRA_LIB_DIRS})
message(STATUS "Found extra libraries at: ${EXTRA_LIB_DIRS}")

# Backward debugging is available only in Linux
if(LINUX)
  add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/lib/backward")
endif()

# Now adding source code
add_subdirectory(src)
