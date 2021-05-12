set(SOURCE_FILES
  Common.cpp
  Debug.cpp
  Normalizer.cpp
  InlineSimpleFunction.cpp
  ElimUnusedAuxFunction.cpp
  ElimUnusedGlobal.cpp
  ElimIdenticalInstrs.cpp
  ElimAllocaStoreArg.cpp
  CombineGEP.cpp
  InitGlobal.cpp
  UninlineInstruction.cpp)

# Relevant LLVM Components can be found by: llvm-config --components
llvm_map_components_to_libnames(LLVM_LIBS
  asmparser core bitreader
  bitwriter irreader ipo scalaropts)

# add_library(LLVMUninlineConstExpr MODULE UninlineConstExpr.cpp)

if (APPLE)
  # compile to a binary file
  add_executable(normalizer ${SOURCE_FILES})
else()
  # compile to a binary file
  add_executable(normalizer ${SOURCE_FILES} ${BACKWARD_ENABLE})
  # add_executable(normalizer ${SOURCE_FILES})
  add_backward(normalizer)
endif()

# set_target_properties(NormalizerPass PROPERTIES
#   COMPILE_FLAGS "-fno-rtti"
#   )

target_link_libraries(normalizer ${LLVM_LIBS})
# target_link_libraries(Discover ${LLVM_LIBS})
# target_link_libraries (DiscoverStatic ${LLVM_LIBS})

# target_link_libraries(LLVMUninlineConstExpr ${LLVM_LIBS} )

# Deploy after build
add_custom_command(TARGET normalizer POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  -E copy $<TARGET_FILE:normalizer> ${PROJECT_BINARY_DIR}/normalizer
  COMMENT "Copied to: ${PROJECT_BINARY_DIR}/normalizer")
