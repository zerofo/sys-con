cmake_minimum_required(VERSION 3.14)
project(SysConTests)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
    googletest_1_15_0
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.0  # Adjust this to the version you need
)

set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
#set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest_1_15_0)

enable_testing() #to discover tests in test explorer 

file(GLOB TEST_FILES ${PROJECT_SOURCE_DIR}/*.cpp)

add_executable(SysConTests ${TEST_FILES}) # add this executable

target_link_libraries(SysConTests PRIVATE GTest::gmock_main)
target_link_libraries(SysConTests PRIVATE SysConControllerLib)
target_link_libraries(SysConTests PRIVATE SysConModule)

include(GoogleTest)
gtest_discover_tests(SysConTests) # discovers tests by asking the compiled test executable to enumerate its tests