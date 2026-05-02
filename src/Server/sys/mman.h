// Dummy sys/mman.h for Windows compilation
// The core engine includes this file but doesn't actually call mmap() functions directly
// in the files other than MemoryMappedFile.cpp (which we've replaced).
#pragma once

#define PROT_READ 1
#define MAP_SHARED 1
#define MAP_FAILED ((void*)-1)

// Provide dummy declarations if necessary
