cmake_minimum_required(VERSION 3.13.4)
project(c-compiler)


set(CMAKE_CXX_STANDARD 14)
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

aux_source_directory(./src SRC_LIST)

add_executable(codegen ${SRC_LIST})

llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(codegen ${llvm_libs})
