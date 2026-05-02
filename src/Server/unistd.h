// Dummy unistd.h for Windows compilation
#pragma once

#include <io.h>
#define F_OK 0
#define W_OK 2
#define R_OK 4

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

// Map POSIX access to MSVC _access
#define access _access