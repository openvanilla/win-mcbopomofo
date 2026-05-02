#pragma once

#ifdef _MSC_VER
// MSVC's std::string_view uses a class type for iterators, but the core engine 
// sometimes expects const char*.
// To avoid modifying the core engine too much, we used to use macros here, 
// but they interfered with the standard library.
// Instead, we now recommend using 'auto' instead of 'const auto*' in the engine 
// code for better cross-platform compatibility, or using .data() when a pointer 
// is explicitly needed.
#endif
