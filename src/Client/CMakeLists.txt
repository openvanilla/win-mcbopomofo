add_library(McBopomofoTIP SHARED dllmain.cpp McBopomofoTIP.cpp Register.cpp EditSession.cpp StateEditSession.cpp)
target_link_libraries(McBopomofoTIP PRIVATE Common Ole32 OleAut32 Uuid Advapi32)
target_compile_definitions(McBopomofoTIP PRIVATE _USRDLL)

# Generate a .def file to export COM functions
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/McBopomofoTIP.def "LIBRARY McBopomofoTIP\nEXPORTS\n  DllGetClassObject PRIVATE\n  DllCanUnloadNow PRIVATE\n  DllRegisterServer PRIVATE\n  DllUnregisterServer PRIVATE\n")

target_sources(McBopomofoTIP PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/McBopomofoTIP.def)
