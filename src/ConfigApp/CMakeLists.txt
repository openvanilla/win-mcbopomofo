add_executable(McBopomofoConfig main.cpp resource.h McBopomofoConfig.rc ${CMAKE_SOURCE_DIR}/src/Server/Settings.cpp ${CMAKE_SOURCE_DIR}/src/Server/InputController.cpp ${CMAKE_SOURCE_DIR}/src/Server/WindowsKeyBridge.cpp)
target_include_directories(McBopomofoConfig PRIVATE ${CMAKE_SOURCE_DIR}/src/Server/Engine ${CMAKE_SOURCE_DIR}/src/Common ${CMAKE_SOURCE_DIR}/src/Server ${CMAKE_SOURCE_DIR}/third_party/OpenCC/src ${WINMCBOPOMOFO_GENERATED_DIR})
target_link_libraries(McBopomofoConfig PRIVATE Common McBopomofoEngine libopencc Ole32 Comctl32 UxTheme)
if(MSVC)
    target_compile_options(McBopomofoConfig PRIVATE /utf-8)
endif()
set_target_properties(McBopomofoConfig PROPERTIES 
    OUTPUT_NAME "McBopomofoConfig"
    WIN32_EXECUTABLE TRUE
)
