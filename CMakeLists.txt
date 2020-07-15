#
# Author: Ta Quang Trung
# Date:   July 6th, 2020
#

project(llvm-normalizer)

cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 11)
# To support backward debugging trace
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-rdynamic")

# Set cmake dirs
set(
  CMAKE_DIR   "${CMAKE_CURRENT_LIST_DIR}/cmake"
  CACHE PATH  "The the path to the cmake directory")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_DIR})

<<<<<<< HEAD
# predefine LLVM_ROOT
#set(LLVM_ROOT "/usr/local/Cellar/llvm@8/8.0.1_1/")
#set(LLVM_LIBS "/usr/local/Cellar/llvm@8/8.0.1_1/lib/")
=======
# Predefine LLVM_ROOT, prefer LLVM 8.0
# Depending on OSes, the path could be:
#   - /usr/lib/llvm-8/
#   - /usr/local/opt/llvm@8
#   - can also be found by: `llvm-config-8 --libdir`
set(LLVM_ROOT "/usr/lib/llvm-8")

>>>>>>> upstream/master
# Find LLVM
#include(FindLLVM)


#config llvm
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})


# Setting for third-party libraries
set(EXTERNAL_LIB_DIRS "${CMAKE_CURRENT_LIST_DIR}/lib")
message(STATUS "Set lib directory:")
message(STATUS "   ${EXTERNAL_LIB_DIRS}")

<<<<<<< HEAD
# Setting for backtrace
if(NOT APPLE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/lib/backward")
=======
# Setting for the backward tracing package
# NOTE: this package is available only in Linux
if(NOT APPLE)
  add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/lib/backward")
>>>>>>> upstream/master
endif()

# Now adding source code
add_subdirectory(src)
