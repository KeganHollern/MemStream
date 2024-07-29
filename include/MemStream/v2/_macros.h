#pragma once

// definition to enable debug logs in MemProcFS & memstream
#ifdef MEMSTREAM_VERBOSE
#include <print>
#define LOG_ERR(...) std::println(__VA_ARGS__)
#else
#define LOG_ERR(...)
#endif