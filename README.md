llvm-normalizer
--------------------------------

A LLVM bitcode normalizer to support the Discover analyzer

# Prerequisites:

## External libraries:

- For backward-cpp:

  ```
  # install LLVM and Clang 11 (or the suitable version)
  sudo apt-get install llvm-11-dev clang-11 libclang-11-dev

  # install other libraries
  sudo apt-get install libedit-dev

  ```

## This tool:

- LLVM framework


# Installation

- Modify the file `CMakeLists.txt` to set `LLVM_ROOT` to the installation of
  your LLVM library, LLVM 8.0 is preferred. For examples:

  + In Linux: `set(LLVM_ROOT "/usr/local/opt/llvm@8")`

  + In macOS: `set(LLVM_ROOT "/usr/local/Cellar/...")` (Need to update this
    part).


- update
 + Add the llvm bin path to PATH
 + Eg: Add export PATH="/usr/local/opt/llvm/bin:$PATH" in  ~/.bash_profile

- Build by CMake:

  ```
  mkdir build
  cd build
  cmake ../
  make
  ```
