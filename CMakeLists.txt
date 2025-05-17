cmake_minimum_required(VERSION 3.14)
project(SysCon)

#Import the library
add_subdirectory(source/Sysmodule)

#Import the library
add_subdirectory(source/ControllerLib)

#HIDDataInterpreter
add_subdirectory(lib/HIDDataInterpreter/src)

#Import the tests
add_subdirectory(tests)
