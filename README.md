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

- Modify the file `CMakeLists.txt` to set `LLVM_ROOT`

```
mkdir build
cd build
cmake ../
make
```
