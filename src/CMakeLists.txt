set(SOURCE_FILES
  Normalizer.cpp
  UninlineConstExpr.cpp)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${EXTERNAL_LIB_DIRS})
# add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs 
        asmparser core linker bitreader 
        bitwriter irreader ipo scalaropts
        analysis target mc support passes interpreter 
        executionengine nativecodegen )

add_library(LLVMUninlineConstExpr MODULE UninlineConstExpr.cpp)

if (APPLE)
  add_executable(normalizer ${SOURCE_FILES})
else()
  add_executable(normalizer ${SOURCE_FILES} ${BACKWARD_ENABLE})
  add_backward(normalizer)
endif()

target_link_libraries(normalizer ${llvm_libs})
target_link_libraries(LLVMUninlineConstExpr ${llvm_libs} )

# Deploy after build
add_custom_command(TARGET normalizer POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  -E copy $<TARGET_FILE:normalizer> ${PROJECT_BINARY_DIR}/normalizer
  COMMENT "Deployed to:\n${PROJECT_BINARY_DIR}/normalizer")
