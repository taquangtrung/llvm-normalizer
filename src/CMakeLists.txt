set(SOURCE_FILES
  normalizer.cpp
  # NormalizeConstExpr.cpp
  )

include_directories(${LLVM_INCLUDE_DIRS})
# add_definitions(${LLVM_DEFINITIONS})

# add_library(LLVMNormalizeConstExpr MODULE NormalizeConstExpr.cpp)

add_executable(normalizer ${SOURCE_FILES})
target_link_libraries(normalizer ${LLVM_LIBS})
