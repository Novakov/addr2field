cmake_minimum_required(VERSION 3.21)

project(Addr2Field CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Microsoft.GSL REQUIRED)
find_package(libdwarf REQUIRED)
find_package(spdlog REQUIRED)
find_package(Python3 REQUIRED)

enable_testing()

add_subdirectory(execs)
add_subdirectory(libs)
add_subdirectory(tests)
