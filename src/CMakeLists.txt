set(SOURCE_FILES
  normalizer.cpp
  NormalizeConstExpr.cpp)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${EXTERNAL_LIB_DIRS})
# add_definitions(${LLVM_DEFINITIONS})

add_library(LLVMNormalizeConstExpr MODULE NormalizeConstExpr.cpp)

add_executable(normalizer ${SOURCE_FILES} ${BACKWARD_ENABLE})
add_backward(normalizer)
target_link_libraries(normalizer ${LLVM_LIBS})

# Deploy after build
add_custom_command(TARGET normalizer POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  -E copy $<TARGET_FILE:normalizer> ${PROJECT_BINARY_DIR}/normalizer
  COMMENT "Deployed to:\n${PROJECT_BINARY_DIR}/normalizer")
