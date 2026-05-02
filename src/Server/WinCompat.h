#pragma once

#ifdef _WIN32
// Intentionally left blank. The previous macro injection approach caused
// standard library header compilation failures and has been removed.
// We will resolve the path conversion issue by excluding the problematic 
// file (VariantAnnotator.cpp) from the build and rewriting its functionality
// if needed.
#endif