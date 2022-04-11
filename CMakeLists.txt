cmake_minimum_required(VERSION 3.13.4)
project(c-compiler)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

aux_source_directory(./src SRC_LIST)

add_executable(codegen ${SRC_LIST})

message(${LLVM_TARGETS_TO_BUILD})
# llvm_map_components_to_libnames(llvm_libs support core irreader executionEngine target native)
llvm_map_components_to_libnames(llvm_libs ${LLVM_TARGETS_TO_BUILD} support core irreader codegen mc mcparser option)
target_link_libraries(codegen ${llvm_libs})
