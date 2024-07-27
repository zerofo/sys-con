cmake_minimum_required(VERSION 3.14)
project(SysConControllerLib VERSION 1.0.0)

set (CMAKE_CXX_STANDARD 20)

#List of sources
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/Controllers/*.cpp)

file(GLOB HEADERS_FILES ${PROJECT_SOURCE_DIR}/Controllers/*.h)

if (WIN32)
	#Add filters in visual studios
	source_group("src"          FILES ${SRC_FILES})
	source_group("headers"      FILES ${HEADERS_FILES} )

	#Remove zero check project
	set(CMAKE_SUPPRESS_REGENERATION true)
endif()

add_library(SysConControllerLib ${SRC_FILES} ${HEADERS_FILES})

target_link_libraries(SysConControllerLib PRIVATE HIDDataInterpreterLib)

target_include_directories(SysConControllerLib PUBLIC ${PROJECT_SOURCE_DIR}/)
