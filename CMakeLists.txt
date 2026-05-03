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
    add_compile_options(/W4 /WX- /EHsc /utf-8 /FS)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)

    # Use Static CRT (/MT) for Release builds to avoid VCRUNTIME140.dll dependency issues in COM host processes
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_subdirectory(src)

# Add tests
enable_testing()
add_executable(CandidateWindowTest tests/test_candidate_window.cpp src/Client/CandidateWindow.cpp src/Common/UTFHelper.cpp)
target_include_directories(CandidateWindowTest PRIVATE ${CMAKE_SOURCE_DIR}/src/Client ${CMAKE_SOURCE_DIR}/src/Common ${CMAKE_SOURCE_DIR}/src/Engine)
target_link_libraries(CandidateWindowTest PRIVATE user32 gdi32 d2d1 dwrite advapi32 ole32 shell32)
add_test(NAME CandidateWindowTest COMMAND CandidateWindowTest)
