# Paths to engine
set(SERVER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ENGINE_DIR ${SERVER_DIR}/Engine)

# Core Engine static library
add_library(McBopomofoEngine STATIC
    ${ENGINE_DIR}/AssociatedPhrasesV2.cpp
    ${ENGINE_DIR}/ByteBlockBackedDictionary.cpp
    ${ENGINE_DIR}/McBopomofoLM.cpp
    ${SERVER_DIR}/MemoryMappedFileWin.cpp
    ${ENGINE_DIR}/ParselessLM.cpp
    ${ENGINE_DIR}/ParselessPhraseDB.cpp
    ${ENGINE_DIR}/PhraseReplacementMap.cpp
    ${ENGINE_DIR}/UserOverrideModel.cpp
    ${ENGINE_DIR}/UserPhrasesLM.cpp
    ${ENGINE_DIR}/UTF8Helper.cpp
    ${ENGINE_DIR}/gramambular2/reading_grid.cpp
    ${ENGINE_DIR}/Mandarin/Mandarin.cpp
    ${SERVER_DIR}/InputState.cpp
    ${SERVER_DIR}/KeyHandler.cpp
    ${SERVER_DIR}/Log.cpp
    ${ENGINE_DIR}/VariantAnnotator.cpp
    ${SERVER_DIR}/Big5Utils/Big5Utils.cpp
    ${SERVER_DIR}/ChineseNumbers/ChineseNumbers.cpp
    ${SERVER_DIR}/ChineseNumbers/StringUtils.cpp
    ${SERVER_DIR}/ChineseNumbers/SuzhouNumbers.cpp
    ${SERVER_DIR}/RomanNumbers/RomanNumbers.cpp
    ${SERVER_DIR}/NumberInputHelper.cpp
    ${SERVER_DIR}/DictionaryService.cpp
    ${SERVER_DIR}/InputMacro.cpp
)

# Include directories for the engine
target_include_directories(McBopomofoEngine PUBLIC
    ${SERVER_DIR} # For sys/mman.h and sibling utility folders
    ${ENGINE_DIR}
    ${ENGINE_DIR}/gramambular2
    ${ENGINE_DIR}/Mandarin
    ${CMAKE_SOURCE_DIR}/src/Common
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
    resource.h
    McBopomofoServer.rc
    CandidateWindowColors.cpp
    InputController.cpp
    WindowsKeyBridge.cpp
    Settings.cpp
    SettingsApp.cpp
)
set_target_properties(McBopomofoServer PROPERTIES WIN32_EXECUTABLE TRUE)
target_include_directories(McBopomofoServer PRIVATE ${CMAKE_SOURCE_DIR}/third_party/OpenCC/src)
target_link_libraries(McBopomofoServer PRIVATE McBopomofoEngine Common libopencc dwmapi windowsapp icuuc icuin)
