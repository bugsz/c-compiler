cmake_minimum_required(VERSION 3.13.4)
project(c-compiler)
set(PROJECT_NAME c-compiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

find_package(LLVM 13 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using LLVM include path ${LLVM_INCLUDE_DIRS}")
message(STATUS "Using LLVM lib path ${LLVM_LIBRARY_DIR}")

find_library(LIBC c)
message(STATUS "Found libc ${LIBC}")

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIR})


message(STATUS "Source dir: ${PROJECT_SOURCE_DIR}")

if ("${ALL}" STREQUAL "ON") 
    message(STATUS "Building all")
    execute_process(COMMAND sh run-script.sh WORKING_DIRECTORY ../src/frontend)
    execute_process(COMMAND ./configure WORKING_DIRECTORY ../src/frontend)
    execute_process(COMMAND make -j8 WORKING_DIRECTORY ../src/frontend)
endif("${ALL}" STREQUAL "ON")


execute_process(COMMAND mkdir build WORKING_DIRECTORY ../src/backend)
execute_process(COMMAND cmake .. WORKING_DIRECTORY ../src/backend/build)
execute_process(COMMAND make -j8 WORKING_DIRECTORY ../src/backend/build)

find_library(ZJUCC_LIB_FRONTEND libzjucc.a ./src/frontend)
find_library(ZJUCC_LIB_BACKEND libzjucc_backend.a ./src/backend)

add_compile_options(-Wno-unused-result)
add_compile_definitions(LIBC_DIR="${LIBC}")
SET(CMAKE_EXE_LINKER_FLAGS "-Wno-unused-result")

add_executable(${PROJECT_NAME} ./src/wrapper.cpp)
target_link_libraries(${PROJECT_NAME} LLVM)
target_link_libraries(${PROJECT_NAME} ${ZJUCC_LIB_FRONTEND})
target_link_libraries(${PROJECT_NAME} ${ZJUCC_LIB_BACKEND})
target_link_libraries(${PROJECT_NAME} ${ZJUCC_LIB_BACKEND})
