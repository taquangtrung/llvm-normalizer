set(SOURCE_FILES
  Normalizer.cpp
  UninlineConstExpr.cpp)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${EXTERNAL_LIB_DIRS})
# add_definitions(${LLVM_DEFINITIONS})

add_library(LLVMUninlineConstExpr MODULE UninlineConstExpr.cpp)

if (APPLE)
  add_executable(normalizer ${SOURCE_FILES})
else()
  add_executable(normalizer ${SOURCE_FILES} ${BACKWARD_ENABLE})
  add_backward(normalizer)
endif()

target_link_libraries(normalizer ${LLVM_LIBS})

# Deploy after build
add_custom_command(TARGET normalizer POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  -E copy $<TARGET_FILE:normalizer> ${PROJECT_BINARY_DIR}/normalizer
  COMMENT "Deployed to:\n${PROJECT_BINARY_DIR}/normalizer")
