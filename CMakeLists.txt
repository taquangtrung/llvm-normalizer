cmake_minimum_required(VERSION 3.0.0)

# set cmake dirs
set(
  CMAKE_DIR   "${CMAKE_CURRENT_LIST_DIR}/cmake"
  CACHE PATH  "The the path to the cmake directory")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_DIR})

# predefine LLVM_ROOT
set(LLVM_ROOT "/usr/lib/llvm-8")

# find llvm, clang
include(FindLLVM)
# include(FindClang)

add_subdirectory(src)
