add_library(McBopomofoTIP SHARED dllmain.cpp Globals.cpp McBopomofoTIP.cpp Register.cpp EditSession.cpp StateEditSession.cpp TsfUiElement.cpp CandidateWindow.cpp TooltipWindow.cpp DisplayAttributeInfo.cpp LangBarButton.cpp McBopomofoTIP.rc)
set_target_properties(McBopomofoTIP PROPERTIES OUTPUT_NAME "McBopomofoTIP_v2")
target_include_directories(McBopomofoTIP PRIVATE ${CMAKE_SOURCE_DIR}/src/Server/Engine ${CMAKE_SOURCE_DIR}/src/Server)
target_link_libraries(McBopomofoTIP PRIVATE Common Ole32 OleAut32 Uuid Advapi32 d2d1 dwrite)
target_compile_definitions(McBopomofoTIP PRIVATE _USRDLL)

# Generate a .def file to export COM functions
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/McBopomofoTIP.def "LIBRARY McBopomofoTIP_v2\nEXPORTS\n  DllGetClassObject PRIVATE\n  DllCanUnloadNow PRIVATE\n  DllRegisterServer PRIVATE\n  DllUnregisterServer PRIVATE\n")

target_sources(McBopomofoTIP PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/McBopomofoTIP.def)
