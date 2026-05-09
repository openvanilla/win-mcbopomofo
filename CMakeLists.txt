cmake_minimum_required(VERSION 3.20)
project(WinMcBopomofo VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Common settings for MSVC
if(MSVC)
    add_compile_options(/W4 /WX- /EHsc /utf-8 /FS /GR)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)

    # Use Static CRT (/MT) for Release builds to avoid VCRUNTIME140.dll dependency issues in COM host processes
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Add external dependencies first
set(BUILD_TESTING OFF CACHE BOOL "Disable third-party tests by default" FORCE)
set(ENABLE_GETTEXT OFF CACHE BOOL "Disable Gettext for OpenCC" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build OpenCC as static" FORCE)
set(USE_SYSTEM_DARTS OFF CACHE BOOL "Use bundled darts" FORCE)
set(USE_SYSTEM_MARISA OFF CACHE BOOL "Use bundled marisa" FORCE)
set(USE_SYSTEM_TINYXML2 OFF CACHE BOOL "Use bundled tinyxml2" FORCE)
set(BUILD_DOCUMENTATION OFF CACHE BOOL "Do not build OpenCC docs" FORCE)
set(ENABLE_GTEST ON CACHE BOOL "Enable GTest" FORCE)
add_subdirectory(third_party/OpenCC EXCLUDE_FROM_ALL)

add_subdirectory(src)

# Add tests
set(BUILD_TESTING ON CACHE BOOL "Enable project tests" FORCE)
enable_testing()
add_executable(CandidateWindowTest tests/test_candidate_window.cpp src/Client/CandidateWindow.cpp src/Common/UTFHelper.cpp)
target_include_directories(CandidateWindowTest PRIVATE ${CMAKE_SOURCE_DIR}/src/Client ${CMAKE_SOURCE_DIR}/src/Common ${CMAKE_SOURCE_DIR}/src/Server/Engine ${CMAKE_SOURCE_DIR}/src/Server)
target_link_libraries(CandidateWindowTest PRIVATE user32 gdi32 d2d1 dwrite advapi32 ole32 shell32)
add_test(NAME CandidateWindowTest COMMAND CandidateWindowTest)

# Add BugReproTest
add_executable(BugReproTest 
    tests/BugReproTest.cpp 
    src/Server/InputController.cpp
)
target_include_directories(BugReproTest PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/Client 
    ${CMAKE_SOURCE_DIR}/src/Common 
    ${CMAKE_SOURCE_DIR}/src/Server/Engine
    ${CMAKE_SOURCE_DIR}/src/Server
    ${CMAKE_SOURCE_DIR}/third_party/OpenCC/src
    ${CMAKE_SOURCE_DIR}/third_party/OpenCC/deps/googletest-1.15.0/googletest/include
)
target_link_libraries(BugReproTest PRIVATE 
    McBopomofoEngine 
    Common 
    libopencc 
    gtest_main
    shell32
)
add_test(NAME BugReproTest COMMAND BugReproTest)

add_executable(IpcTest tests/IpcTest.cpp)
target_include_directories(IpcTest PRIVATE ${CMAKE_SOURCE_DIR}/src/Common ${CMAKE_SOURCE_DIR}/third_party/OpenCC/deps/googletest-1.15.0/googletest/include)
target_link_libraries(IpcTest PRIVATE Common gtest_main)
add_test(NAME IpcTest COMMAND IpcTest)

add_executable(PipeTest tests/PipeTest.cpp)
target_include_directories(PipeTest PRIVATE ${CMAKE_SOURCE_DIR}/src/Common ${CMAKE_SOURCE_DIR}/third_party/OpenCC/deps/googletest-1.15.0/googletest/include)
target_link_libraries(PipeTest PRIVATE Common gtest_main)
add_test(NAME PipeTest COMMAND PipeTest)
