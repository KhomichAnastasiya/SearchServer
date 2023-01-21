cmake_minimum_required(VERSION 3.23.2)

project(search-server)

set(SOURCE_FOLDER "source")
set(HDR_FOLDER "include")

include_directories(HDR_FOLDER)

file(GLOB_RECURSE SRC ${SOURCE_FOLDER}/*.cpp)
file(GLOB_RECURSE HDR ${HDR_FOLDER}/*.h)

add_executable(search-server ${SRC} ${HDR})

target_link_libraries(search-server)