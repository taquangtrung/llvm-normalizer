set(SOURCE_FILES
  Normalizer.cpp
  UnwrapGEP.cpp
  UninlineConstExpr.cpp)

# Relevant LLVM Components can be found by: llvm-config --components
llvm_map_components_to_libnames(LLVM_LIBS
  asmparser core bitreader
  bitwriter irreader ipo scalaropts)

add_library(LLVMUninlineConstExpr MODULE UninlineConstExpr.cpp)

if (APPLE)
  add_executable(normalizer ${SOURCE_FILES})
else()
  add_executable(normalizer ${SOURCE_FILES} ${BACKWARD_ENABLE})
  add_backward(normalizer)
endif()

target_link_libraries(normalizer ${LLVM_LIBS})
target_link_libraries(LLVMUninlineConstExpr ${LLVM_LIBS} )

# Deploy after build
add_custom_command(TARGET normalizer POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  -E copy $<TARGET_FILE:normalizer> ${PROJECT_BINARY_DIR}/normalizer
  COMMENT "Copied to: ${PROJECT_BINARY_DIR}/normalizer")
