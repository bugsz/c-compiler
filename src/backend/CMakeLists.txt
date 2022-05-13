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
message(STATUS "Using LLVM path ${LLVM_INCLUDE_DIRS}")


include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

find_library(ZJUCC_LIB libzjucc.a ../frontend)

aux_source_directory(./src SRC_LIST)
aux_source_directory(./src/ast SRC_LIST)
message(STATUS "SRC_LIST: ${SRC_LIST}")


include_directories(./include)
include_directories(../frontend)

add_compile_options(-Wno-unused-result)
add_library(${PROJECT_NAME} ${SRC_LIST})
message(${LLVM_TARGETS_TO_BUILD})
# llvm_map_components_to_libnames(llvm_libs support core irreader executionEngine target native)
llvm_map_components_to_libnames(llvm_libs ${LLVM_TARGETS_TO_BUILD} support core irreader codegen mc mcparser option)

# SET(CMAKE_EXE_LINKER_FLAGS "-Wl,-znodelete")
SET(CMAKE_EXE_LINKER_FLAGS "-Wno-unused-result")

target_link_libraries(${PROJECT_NAME} ${llvm_libs})
target_link_libraries(${PROJECT_NAME} ${ZJUCC_LIB})