#ifndef V_FRAMEWORK_CORE_PLATFORM_DEFAULT_H
#define V_FRAMEWORK_CORE_PLATFORM_DEFAULT_H

//////////////////////////////////////////////////////////////////////////
// Platforms

#include "platform_restricted_file_default.h"

#if defined(__clang__)
    #define V_COMPILER_CLANG   __clang_major__
#elif defined(_MSC_VER)
    #define V_COMPILER_MSVC    _MSC_VER
#else
#   error This compiler is not supported
#endif

#include "vcore/traits_platform.h"


//////////////////////////////////////////////////////////////////////////

#define V_INLINE                       inline
#define V_THREAD_LOCAL                 V_TRAIT_COMPILER_THREAD_LOCAL
#define V_DYNAMIC_LIBRARY_PREFIX       V_TRAIT_OS_DYNAMIC_LIBRARY_PREFIX
#define V_DYNAMIC_LIBRARY_EXTENSION    V_TRAIT_OS_DYNAMIC_LIBRARY_EXTENSION

#if defined(V_COMPILER_CLANG)
    #define V_DLL_EXPORT               V_TRAIT_OS_DLL_EXPORT_CLANG
    #define V_DLL_IMPORT               V_TRAIT_OS_DLL_IMPORT_CLANG
#elif defined(V_COMPILER_MSVC)
    #define V_DLL_EXPORT               __declspec(dllexport)
    #define V_DLL_IMPORT               __declspec(dllimport)
#endif

// These defines will be deprecated in the future with LY-99152
#if defined(V_PLATFORM_MAC)
    #define V_PLATFORM_APPLE_OSX
#endif
#if defined(V_PLATFORM_IOS)
    #define V_PLATFORM_APPLE_IOS
#endif
#if V_TRAIT_OS_PLATFORM_APPLE
    #define V_PLATFORM_APPLE
#endif

#if V_TRAIT_OS_HAS_DLL_SUPPORT
    #define V_HAS_DLL_SUPPORT
#endif

/// Deprecated macro
#define V_DEPRECATED(_decl, _message) [[deprecated(_message)]] _decl

#define V_STRINGIZE_I(text) #text

#if defined(V_COMPILER_MSVC)
#    define V_STRINGIZE(text) V_STRINGIZE_A((text))
#    define V_STRINGIZE_A(arg) V_STRINGIZE_I arg
#else
#    define V_STRINGIZE(text) V_STRINGIZE_I(text)
#endif

#if defined(V_COMPILER_MSVC)

/// Disables a warning using push style. For use matched with an V_POP_WARNING
#define V_PUSH_DISABLE_WARNING(_msvcOption, __)    \
    __pragma(warning(push))                         \
    __pragma(warning(disable : _msvcOption))

/// Pops the warning stack. For use matched with an V_PUSH_DISABLE_WARNING
#define V_POP_DISABLE_WARNING                      \
    __pragma(warning(pop))


/// Classes in Editor Sandbox and Tools which dll export there interfaces, but inherits from a base class that doesn't dll export
/// will trigger a warning
#define V_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING V_PUSH_DISABLE_WARNING(4275, "-Wunknown-warning-option")
#define V_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING V_POP_DISABLE_WARNING
/// Disables a warning for dll exported classes which has non dll-exported members as this can cause ABI issues if the layout of those classes differs between dlls.
/// QT classes such as QList, QString, QMap, etc... and Cry Math classes such Vec3, Quat, Color don't dllexport their interfaces
/// Therefore this macro can be used to disable the warning when caused by 3rdParty libraries
#define V_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING V_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#define V_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING V_POP_DISABLE_WARNING

#   define V_FORCE_INLINE  __forceinline

/// Pointer will be aliased.
#   define V_MAY_ALIAS
/// Function signature macro
#   define V_FUNCTION_SIGNATURE    __FUNCSIG__

//////////////////////////////////////////////////////////////////////////
#elif defined(V_COMPILER_CLANG)

/// Disables a single warning using push style. For use matched with an V_POP_WARNING
#define V_PUSH_DISABLE_WARNING(__, _clangOption)           \
    _Pragma("clang diagnostic push")                        \
    _Pragma(V_STRINGIZE(clang diagnostic ignored _clangOption))

/// Pops the warning stack. For use matched with an V_PUSH_DISABLE_WARNING
#define V_POP_DISABLE_WARNING                              \
    _Pragma("clang diagnostic pop")

#define V_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
#define V_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
#define V_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#define V_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

#   define V_FORCE_INLINE  inline

/// Pointer will be aliased.
#   define V_MAY_ALIAS __attribute__((__may_alias__))
/// Function signature macro
#   define V_FUNCTION_SIGNATURE    __PRETTY_FUNCTION__

#else
    #error Compiler not supported
#endif

// We need to define V_DEBUG_BUILD in debug mode. We can also define it in debug optimized mode (left up to the user).
// note that _DEBUG is not in fact always defined on all platforms, and only V_DEBUG_BUILD should be relied on.
#if !defined(V_DEBUG_BUILD) && defined(_DEBUG)
#   define V_DEBUG_BUILD
#endif

#if !defined(V_PROFILE_BUILD) && defined(_PROFILE)
#   define V_PROFILE_BUILD
#endif

// note that many include ONLY PlatformDef.h and not base.h, so flags such as below need to be here.
// V_ENABLE_DEBUG_TOOLS - turns on and off interaction with the debugger.
// Things like being able to check whether the current process is being debugged, to issue a "debug break" command, etc.
#if defined(V_DEBUG_BUILD) && !defined(V_ENABLE_DEBUG_TOOLS)
#   define V_ENABLE_DEBUG_TOOLS
#endif

// V_ENABLE_TRACE_ASSERTS - toggles display of native UI assert dialogs with ignore/break options
#define V_ENABLE_TRACE_ASSERTS 1

// V_ENABLE_TRACING - turns on and off the availability of V_TracePrintf / V_Assert / V_Error / V_Warning
#if (defined(V_DEBUG_BUILD) || defined(V_PROFILE_BUILD)) && !defined(V_ENABLE_TRACING)
#   define V_ENABLE_TRACING
#endif

#if !defined(V_COMMAND_LINE_LEN)
#   define V_COMMAND_LINE_LEN 2048
#endif

#endif  // V_FRAMEWORK_CORE_PLATFORM_DEFAULT_H