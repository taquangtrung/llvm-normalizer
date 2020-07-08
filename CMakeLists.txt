#
# Author: Ta Quang Trung
# Date:   July 6th, 2020
#

cmake_minimum_required(VERSION 3.0.0)

# set cmake dirs
set(
  CMAKE_DIR   "${CMAKE_CURRENT_LIST_DIR}/cmake"
  CACHE PATH  "The the path to the cmake directory")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_DIR})

# predefine LLVM_ROOT
set(LLVM_ROOT "/usr/lib/llvm-8")

# Find LLVM
include(FindLLVM)

# Find other third-party libraries
set(EXTERNAL_LIB_DIRS "${CMAKE_CURRENT_LIST_DIR}/lib/")

add_subdirectory(src)

# TODO: copy binary to the root folder
# config_file()
