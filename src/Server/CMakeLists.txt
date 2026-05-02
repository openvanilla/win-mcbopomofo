# Paths to engine
set(ENGINE_DIR ${CMAKE_SOURCE_DIR}/src/Engine)

# Core Engine static library
add_library(McBopomofoEngine STATIC
    ${ENGINE_DIR}/AssociatedPhrasesV2.cpp
    ${ENGINE_DIR}/ByteBlockBackedDictionary.cpp
    ${ENGINE_DIR}/McBopomofoLM.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MemoryMappedFileWin.cpp
    ${ENGINE_DIR}/ParselessLM.cpp
    ${ENGINE_DIR}/ParselessPhraseDB.cpp
    ${ENGINE_DIR}/PhraseReplacementMap.cpp
    ${ENGINE_DIR}/UserOverrideModel.cpp
    ${ENGINE_DIR}/UserPhrasesLM.cpp
    ${ENGINE_DIR}/UTF8Helper.cpp
    ${ENGINE_DIR}/gramambular2/reading_grid.cpp
    ${ENGINE_DIR}/Mandarin/Mandarin.cpp
)

# Include directories for the engine
target_include_directories(McBopomofoEngine PUBLIC
    ${ENGINE_DIR}
    ${ENGINE_DIR}/gramambular2
    ${ENGINE_DIR}/Mandarin
    ${CMAKE_CURRENT_SOURCE_DIR} # For sys/mman.h
)

# Force C++17 for the engine to maintain compatibility with std::string_view iterators
set_target_properties(McBopomofoEngine PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

# MSVC C++ compatibility for older code
if(MSVC)
    target_compile_options(McBopomofoEngine PRIVATE /Zc:char8_t- /FS /FI"${CMAKE_CURRENT_SOURCE_DIR}/IteratorCompat.h")
endif()

# Server executable
add_executable(McBopomofoServer main.cpp)
target_link_libraries(McBopomofoServer PRIVATE McBopomofoEngine Common)
