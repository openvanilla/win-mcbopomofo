cmake_minimum_required(VERSION 3.12)
project(BopomofoBrailleCpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()


# Library target
add_library(BopomofoBraille STATIC
    src/BopomofoSyllable.cpp
    src/Converter.cpp
    src/Tokens/Digits.cpp
    src/Tokens/FullWidthPunctuation.cpp
    src/Tokens/HalfWidthPunctuation.cpp
    src/Tokens/Letter.cpp
)

target_include_directories(BopomofoBraille PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(BopomofoBraille PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    # Test target
    enable_testing()
    add_subdirectory(tests)
endif()
