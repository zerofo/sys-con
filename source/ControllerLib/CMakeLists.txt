cmake_minimum_required(VERSION 3.14)
project(SysConControllerLib VERSION 1.0.0)

set (CMAKE_CXX_STANDARD 20)

file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/Controllers/*.cpp)
file(GLOB HEADERS_FILES ${PROJECT_SOURCE_DIR}/Controllers/*.h)

add_library(SysConControllerLib ${SRC_FILES} ${HEADERS_FILES})

target_link_libraries(SysConControllerLib PRIVATE HIDDataInterpreterLib)

target_include_directories(SysConControllerLib PUBLIC ${PROJECT_SOURCE_DIR}/)
