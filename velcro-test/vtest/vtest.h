#ifndef V_FRAMEWORK_VTEST_VTEST_H
#define V_FRAMEWORK_VTEST_VTEST_H

#include <vcore/platform_default.h>
#include <vtest_traits_platform.h>
#include <list>
#include <array>

V_PUSH_DISABLE_WARNING(4389 4800, "-Wunknown-warning-option"); // 'int' : forcing value to bool 'true' or 'false' (performance warning).
#undef strdup // This define is required by googletest
#include <gtest/gtest.h>
#include <gmock/gmock.h>
V_POP_DISABLE_WARNING;

#if defined(HAVE_BENCHMARK)
#include <benchmark/benchmark.h>
#endif

#include <vcore/memory/osallocator.h>

#define VTEST_DLL_PUBLIC V_DLL_EXPORT
#define VTEST_EXPORT extern "C" VTEST_DLL_PUBLIC


#endif // V_FRAMEWORK_VTEST_VTEST_H