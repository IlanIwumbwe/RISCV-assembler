cmake_minimum_required(VERSION 3.10)  
project(assembler)     

set(CMAKE_CXX_STANDARD 20)             
set(CMAKE_CXX_STANDARD_REQUIRED ON)  
set(CMAKE_CXX_FLAGS_DEBUG "-g -O3 -DDEBUG -Wall -Wextra -Wswitch-enum")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")


file(GLOB_RECURSE SRCS "src/*.cpp")

add_executable(assembler ${SRCS})

target_include_directories(assembler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

