cmake_minimum_required(VERSION 3.5)
project(sender CXX)

message("project name is: "${PROJECT_NAME})
message("project dir is: "${CMAKE_CURRENT_SOURCE_DIR})

#include(CheckIncludeFileCXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_PREFIX_PATH "/lib/x86_64-linux-gnu/")

find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)
find_package(gflags REQUIRED)
#link_directories("/usr/local/lib/")
#find_package(jemalloc REQUIRED)

set(SOURCES main.cc ${CMAKE_CURRENT_SOURCE_DIR}/protocol/ColValue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/util/ArrowUtil.h
    ${CMAKE_CURRENT_SOURCE_DIR}/util/RandomUtil.h
    ${CMAKE_CURRENT_SOURCE_DIR}/util/ValidUtil.h)


LINK_DIRECTORIES(/usr/local/lib)

add_executable(${PROJECT_NAME} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
COMPILE_FLAGS "--std=c++17")

target_link_libraries(${PROJECT_NAME} parquet arrow)
target_link_libraries(${PROJECT_NAME} -static-libgcc -static-libstdc++)
target_link_libraries(${PROJECT_NAME} -static-libgcc -static-libstdc++)
target_link_libraries(${PROJECT_NAME} gflags.a)
target_link_libraries(${PROJECT_NAME} jemalloc.a)
target_link_libraries(${PROJECT_NAME} -lspdlog)
target_link_libraries(${PROJECT_NAME} zstd.a)
target_link_libraries(${PROJECT_NAME} -lpthread)
target_link_libraries(${PROJECT_NAME} -ldl)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/protocol
    ${CMAKE_CURRENT_SOURCE_DIR}/util)

###
set(PROJECT_NAME2 receiver)


add_executable(${PROJECT_NAME2} main_recv.cc)

set_target_properties(${PROJECT_NAME2} PROPERTIES
COMPILE_FLAGS "--std=c++17")


target_link_libraries(${PROJECT_NAME2} parquet arrow)
target_link_libraries(${PROJECT_NAME2} -static-libgcc -static-libstdc++)
target_link_libraries(${PROJECT_NAME2} -static-libgcc -static-libstdc++)
target_link_libraries(${PROJECT_NAME2} gflags.a)
target_link_libraries(${PROJECT_NAME2} jemalloc.a)
target_link_libraries(${PROJECT_NAME2} spdlog)
target_link_libraries(${PROJECT_NAME2} zstd.a)
target_link_libraries(${PROJECT_NAME2} -lpthread)
target_link_libraries(${PROJECT_NAME2} -ldl)

target_include_directories(${PROJECT_NAME2}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/protocol
    ${CMAKE_CURRENT_SOURCE_DIR}/util)

