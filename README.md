llvm-normalizer
--------------------------------

A LLVM bitcode normalizer to support the Discover analyzer

# Prerequisites:

## External libraries:

- For backward-cpp:

  ```
  sudo apt-get install libdw-dev libelf-dev libdwarf-dev
  ```

## This tool:

- LLVM framework


# Installation

- Modify the file `CMakeLists.txt` to set `LLVM_ROOT` to the installation of
  your LLVM library, LLVM 8.0 is preferred. For examples:

  + In Linux: `set(LLVM_ROOT "/usr/local/opt/llvm@8")`

  + In macOS: `set(LLVM_ROOT "/usr/local/Cellar/...")` (Need to update this
    part).

- Build by CMake:

  ```
  mkdir build
  cd build
  cmake ../
  make
  ```
