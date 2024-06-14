#include <core/compression/compression.h>
#include <core/memory/system_allocator.h>

using namespace V;

//////////////////////////////////////////////////////////////////////////
// ZLib
#define NO_GZIP
#include <zlib.h>