cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_GENERATOR "Ninja")
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "build type" FORCE)

set(BUILD_STATIC ON CACHE BOOL "build static" FORCE)
set(BUNDLE_DEPENDENCIES ON CACHE BOOL "bundle deps" FORCE)

project(REST VERSION 1.0 LANGUAGES C CXX)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/mysql-connector)

file(GLOB_RECURSE sources ${CMAKE_CURRENT_LIST_DIR}/src/*.c ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)

set(CMAKE_CXX_STANDARD 23)

add_executable(${PROJECT_NAME} ${sources})
target_include_directories(${PROJECT_NAME} PUBLIC include ${CMAKE_BINARY_DIR}/include/mysqlx)
target_link_libraries(${PROJECT_NAME} PUBLIC connector)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_CURRENT_LIST_DIR})