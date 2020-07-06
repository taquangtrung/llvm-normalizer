set(sources normalizer.cpp)

add_executable(normalizer ${sources})

include_directories(${LLVM_INCLUDE_DIRS})
# add_definitions(${LLVM_DEFINITIONS})
target_link_libraries(normalizer ${LLVM_LIBS})
