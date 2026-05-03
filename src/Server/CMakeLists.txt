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
    ${ENGINE_DIR}/InputState.cpp
    ${ENGINE_DIR}/KeyHandler.cpp
    ${ENGINE_DIR}/Log.cpp
    ${ENGINE_DIR}/VariantAnnotator.cpp
    ${ENGINE_DIR}/Big5Utils/Big5Utils.cpp
    ${ENGINE_DIR}/ChineseNumbers/ChineseNumbers.cpp
    ${ENGINE_DIR}/ChineseNumbers/StringUtils.cpp
    ${ENGINE_DIR}/ChineseNumbers/SuzhouNumbers.cpp
    ${ENGINE_DIR}/NumberInputHelper.cpp
    ${ENGINE_DIR}/DictionaryService.cpp
    ${ENGINE_DIR}/InputMacro.cpp
)

# Include directories for the engine
target_include_directories(McBopomofoEngine PUBLIC
    ${ENGINE_DIR}
    ${ENGINE_DIR}/gramambular2
    ${ENGINE_DIR}/Mandarin
    ${CMAKE_CURRENT_SOURCE_DIR} # For sys/mman.h
    ${CMAKE_SOURCE_DIR}/third_party/OpenCC/src
)

# Force C++17 for the engine to maintain compatibility with std::string_view iterators
set_target_properties(McBopomofoEngine PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

# MSVC C++ compatibility for older code
if(MSVC)
    target_compile_options(McBopomofoEngine PRIVATE /Zc:char8_t- /FS /FI"${CMAKE_CURRENT_SOURCE_DIR}/IteratorCompat.h")
endif()

# Server executable
add_executable(McBopomofoServer 
    main.cpp
    InputController.cpp
    WindowsKeyBridge.cpp
    Settings.cpp
)
target_include_directories(McBopomofoServer PRIVATE ${CMAKE_SOURCE_DIR}/third_party/OpenCC/src)
target_link_libraries(McBopomofoServer PRIVATE McBopomofoEngine Common libopencc)
