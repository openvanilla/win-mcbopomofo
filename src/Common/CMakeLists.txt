add_library(Common STATIC dummy.cpp UTFHelper.cpp Ipc.cpp NamedPipe.cpp)
target_include_directories(Common PUBLIC .)
