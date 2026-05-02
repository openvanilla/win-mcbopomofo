add_library(McBopomofoTIP SHARED dllmain.cpp)
target_link_libraries(McBopomofoTIP PRIVATE Common)
target_compile_definitions(McBopomofoTIP PRIVATE _USRDLL)
