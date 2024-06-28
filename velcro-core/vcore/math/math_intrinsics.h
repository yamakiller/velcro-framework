#ifndef V_FRAMEWORK_CORE_MATH_MATH_INTRINSICS_H
#define V_FRAMEWORK_CORE_MATH_MATH_INTRINSICS_H

#if defined(V_COMPILER_MSVC)
    #define v_ctz_u32(x)    _tzcnt_u32(x)
    #define v_ctz_u64(x)    _tzcnt_u64(x)
    #define v_clz_u32(x)    _lzcnt_u32(x)
    #define v_clz_u64(x)    _lzcnt_u64(x)
    #define v_popcnt_u32(x) __popcnt(x)
    #define v_popcnt_u64(x) __popcnt64(x)
#elif defined(V_COMPILER_CLANG)
    #define v_ctz_u32(x)    __builtin_ctz(x)
    #define v_ctz_u64(x)    __builtin_ctzll(x)
    #define v_clz_u32(x)    __builtin_clz(x)
    #define v_clz_u64(x)    __builtin_clzll(x)
    #define v_popcnt_u32(x) __builtin_popcount(x)
    #define v_popcnt_u64(x) __builtin_popcountll(x)
#else
    #error Count Leading Zeros, Count Trailing Zeros and Pop Count intrinsics isn't supported for this compiler
#endif

#endif // V_FRAMEWORK_CORE_MATH_MATH_INTRINSICS_H