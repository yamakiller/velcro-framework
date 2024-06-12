#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_CONFIG_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_CONFIG_H

#include <core/std/base.h>

// Compilers are more consistent with the traits, since AZStd doesn't have the goal or design need
// to control that we are pulling std one into AZStd till we have all tests pass and have consisten syntax
// across compilers
#include <type_traits>

//
// define VSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
// when we want to test __stdcall etc function types with is_function etc
//
#if defined(_MSC_EXTENSIONS)
    #define VSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
#else
    #define VSTD_DISABLE_WIN32
#endif

//
// whenever we have a conversion function with elipses
// it needs to be declared __cdecl to suppress compiler
// warnings from MS
#if defined(V_COMPILER_MSVC) && !defined(VSTD_DISABLE_WIN32)
    #define VSTD_TYPE_TRAITS_DECL __cdecl
#else
    #define VSTD_TYPE_TRAITS_DECL /**/
#endif

namespace VStd {
    using std::is_constructible;
    using std::is_default_constructible;
    using std::is_assignable;
    //using std::is_trivially_constructible; still not implemented in GCC 4.9 Android
    //using std::is_trivially_assignable; still not implemented in GCC 4.9 Android

    using std::is_copy_constructible;
    using std::is_copy_assignable;
    //using std::is_trivially_copy_constructible; still not implemented in GCC 4.9 Android
    //using std::is_trivially_copy_assignable; still not implemented in GCC 4.9 Android

    using std::is_move_constructible;
    using std::is_move_assignable;
    //using std::is_trivially_move_constructible; still not implemented in GCC 4.9 Android
    //using std::is_trivially_move_assignable; still not implemented in GCC 4.9 Android

    using std::is_destructible;
    //using std::is_trivially_destructible; still not implemented in GCC 4.9 Android

} // namespace VStd




#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_CONFIG_H