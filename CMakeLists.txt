#
# Author: Ta Quang Trung
# Date:   July 6th, 2020
#

cmake_minimum_required(VERSION 3.0.0)

# To support backward debugging trace
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-rdynamic")

# set cmake dirs
set(
  CMAKE_DIR   "${CMAKE_CURRENT_LIST_DIR}/cmake"
  CACHE PATH  "The the path to the cmake directory")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_DIR})

# predefine LLVM_ROOT
set(LLVM_ROOT "/usr/lib/llvm-8")

# Find LLVM
include(FindLLVM)

# Setting for third-party libraries
set(EXTERNAL_LIB_DIRS "${CMAKE_CURRENT_LIST_DIR}/lib")
message(STATUS "Set lib directory:")
message(STATUS "   ${EXTERNAL_LIB_DIRS}")

# Setting for backtrace
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/lib/backward")

# Now adding source code
add_subdirectory(src)
