cmake_minimum_required(VERSION 3.13.4)
project(c-compiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR})
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(PROJECT_NAME zjucc_backend)

find_package(LLVM 13 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using LLVM include path ${LLVM_INCLUDE_DIRS}")
message(STATUS "Using LLVM lib path ${LLVM_LIBRARY_DIR}")

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIR})


include_directories(./include)
include_directories(../frontend)

aux_source_directory(./src SRC_LIST)
aux_source_directory(./src/ast SRC_LIST)
message(STATUS "SRC_LIST: ${SRC_LIST}")

add_compile_options(-Wno-unused-result)
SET(CMAKE_EXE_LINKER_FLAGS "-Wno-unused-result")

add_library(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} LLVM)