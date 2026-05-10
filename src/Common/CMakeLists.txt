add_library(Common STATIC dummy.cpp UTFHelper.cpp Ipc.cpp NamedPipe.cpp SettingsApp.cpp)
target_include_directories(Common PUBLIC .)
target_link_libraries(Common PUBLIC Advapi32)
