cmake_minimum_required(VERSION 3.13.4)
project(c-compiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

link_directories(${LLVM_LIBRARY_DIR})
find_library(ZJUCC_LIB libzjucc.a ./src/frontend)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(./src/backend/include)
include_directories(./src/frontend)

aux_source_directory(./src/backend/src SRC_LIST)
aux_source_directory(./src/backend/src/ast SRC_LIST)
message(STATUS "SRC_LIST: ${SRC_LIST}")

add_compile_options(-Wno-unused-result)
SET(CMAKE_EXE_LINKER_FLAGS "-Wno-unused-result")

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} LLVM)
target_link_libraries(${PROJECT_NAME} ${ZJUCC_LIB})

option(IRONLY "Only output IR codes to stdout" OFF)
if(IRONLY)
    message(STATUS "Only output IR codes to stdout")
    ADD_DEFINITIONS(-DIRONLY)
endif()