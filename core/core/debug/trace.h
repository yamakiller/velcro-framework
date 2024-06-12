#ifndef V_FRAMEWORK_CORE_DEBUG_TRACE_H
#define V_FRAMEWORK_CORE_DEBUG_TRACE_H

#include <core/platform_default.h>
#include <core/base.h>

namespace V
{
    namespace Debug
    {
        namespace Platform
        {
            void OutputToDebugger(const char* window, const char* message);
        }

         /// 全局跟踪器
        extern class Trace      g_tracer;

        enum LogLevel : int
        {
            Disabled = 0,
            Errors = 1,
            Warnings = 2,
            Info = 3
        };

        // 跟踪器
        class Trace
        {
        public:
            static Trace& Instance()    { return g_tracer; }

            // Declare Trace init for assert tracking initializations, and get/setters for verbosity level
            static void Init();
            static void Destroy();
            static int  GetAssertVerbosityLevel();
            static void SetAssertVerbosityLevel(int level);

            /**
            * Returns the default string used for a system window.
            * It can be useful for Trace message handlers to easily validate if the window they received is the fallback window used by this class,
            * or to force a Trace message bus handler to do special processing by using a known, consistent char*
            */
            static const char* GetDefaultSystemWindow();
            static bool IsDebuggerPresent();
            static bool AttachDebugger();
            static bool WaitForDebugger(float timeoutSeconds = -1.f);

            /// True or false if we want to handle system exceptions.
            static void HandleExceptions(bool isEnabled);

            /// Breaks program execution immediately.
            static void Break();

            /// Crash the application
            static void Crash();

            /// Terminates the process with the specified exit code
            static void Terminate(int exitCode);

            /// Indicates if trace logging functions are enabled based on compile mode and cvar logging level
            static bool IsTraceLoggingEnabledForLevel(LogLevel level);

            static void Assert(const char* fileName, int line, const char* funcName, const char* format, ...);
            static void Error(const char* fileName, int line, const char* funcName, const char* window, const char* format, ...);
            static void Warning(const char* fileName, int line, const char* funcName, const char* window, const char* format, ...);
            static void Printf(const char* window, const char* format, ...);

            static void Output(const char* window, const char* message);

            static void PrintCallstack(const char* window, unsigned int suppressCount = 0, void* nativeContext = 0);

            /// PEXCEPTION_POINTERS on Windows, always NULL on other platforms
            static void* GetNativeExceptionInfo();
        };
    }
}

#ifdef V_ENABLE_TRACING
    /**
     * 如果用户忘记编写布尔表达式来断言，这用于防止错误使用.
     * Example:
     *     V_Assert("Fail always"); // <- assertion will never fail
     * Correct usage:
     *     V_Assert(false, "Fail always");
     */
    namespace V {
        namespace TraceInternal {
            enum class ExpressionValidResult { Valid, Invalid_ConstCharPtr, Invalid_ConstCharArray };
            template<typename T>
            struct ExpressionIsValid {
                static constexpr ExpressionValidResult value = ExpressionValidResult::Valid;
            };
            template<>
            struct ExpressionIsValid<const char*&> {
                static constexpr ExpressionValidResult value = ExpressionValidResult::Valid;
            };
            template<unsigned int N>
            struct ExpressionIsValid<const char(&)[N]> {
                static constexpr ExpressionValidResult value = ExpressionValidResult::Invalid_ConstCharArray;
            };
        }
    }

    #define V_TraceFmtCompileTimeCheck(expression, isVaArgs, baseMsg, msg, msgVargs)                                                                                            \
        {                                                                                                                                                                       \
            using namespace V::TraceInternal;                                                                                                                                   \
            /* This is needed for edge cases for expressions containing lambdas, that were unsupported before C++20 */                                                          \
            [[maybe_unused]] const auto& rTraceFmtCompileTimeCheckExpressionHelper = (expression);                                                                              \
            constexpr ExpressionValidResult isValidTraceFmtResult = ExpressionIsValid<decltype(rTraceFmtCompileTimeCheckExpressionHelper)>::value;                              \
            /* Assert different message depending whether it's const char array or if we have extra arguments */                                                                \
            static_assert(!(isVaArgs) ? isValidTraceFmtResult != ExpressionValidResult::Invalid_ConstCharArray : true, baseMsg " " msg);                                        \
            static_assert(isVaArgs  ? isValidTraceFmtResult != ExpressionValidResult::Invalid_ConstCharArray : true, baseMsg " " msgVargs);                                     \
            static_assert(!(isVaArgs) ? isValidTraceFmtResult != ExpressionValidResult::Invalid_ConstCharPtr   : true, baseMsg " " msg " or "#expression" != nullptr");         \
            static_assert(isVaArgs  ? isValidTraceFmtResult != ExpressionValidResult::Invalid_ConstCharPtr   : true, baseMsg " " msgVargs " or "#expression" != nullptr");      \
        }

    #define V_Assert(expression, ...)                                                                                          \
        V_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")                                                               \
        if(!(expression))                                                                                                      \
        {                                                                                                                      \
            V_TraceFmtCompileTimeCheck(expression, V_VA_HAS_ARGS(__VA_ARGS__),                                                 \
            "String used in place of boolean expression for V_Assert.",                                                        \
            "Did you mean V_Assert(false, \"%s\", "#expression"); ?",                                                          \
            "Did you mean V_Assert(false, "#expression", "#__VA_ARGS__"); ?");                                                 \
            V::Debug::Trace::Instance().Assert(__FILE__, __LINE__, V_FUNCTION_SIGNATURE, __VA_ARGS__);                         \
        }                                                                                                                      \
        V_POP_DISABLE_WARNING

    #define V_Error(window, expression, ...)                                                                                   \
        V_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")                                                               \
        if (!(expression))                                                                                                     \
        {                                                                                                                      \
            V_TraceFmtCompileTimeCheck(expression, V_VA_HAS_ARGS(__VA_ARGS__),                                                 \
                "String used in place of boolean expression for V_Error.",                                                     \
                "Did you mean V_Error("#window", false, \"%s\", "#expression"); ?",                                            \
                "Did you mean V_Error("#window", false, "#expression", "#__VA_ARGS__"); ?");                                   \
            V::Debug::Trace::Instance().Error(__FILE__, __LINE__, V_FUNCTION_SIGNATURE, window, __VA_ARGS__);                  \
        }                                                                                                                      \
        V_POP_DISABLE_WARNING


    #define V_CONCAT_VAR_NAME(x, y) x ## y

    //! The V_ErrorOnce macro output the result of the format string only once for use of the macro itself.
    //! It does not take into account the result of the format string to determine whether to output the string or not
    //! What this means is that if the formatting results in different output string result only the first result
    //! will ever be output
    #define V_ErrorOnce(window, expression, ...)                                                                                \
        V_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")                                                                \
        if (!(expression))                                                                                                      \
        {                                                                                                                       \
            V_TraceFmtCompileTimeCheck(expression, V_VA_HAS_ARGS(__VA_ARGS__),                                                  \
                "String used in place of boolean expression for V_ErrorOnce.",                                                  \
                "Did you mean V_ErrorOnce("#window", false, \"%s\", "#expression"); ?",                                         \
                "Did you mean V_ErrorOnce("#window", false, "#expression", "#__VA_ARGS__"); ?");                                \
            static bool V_CONCAT_VAR_NAME(vErrorDisplayed, __LINE__) = false;                                                   \
            if (V_DISABLE_COPY(vErrorDisplayed, __LINE__))                                                                      \
            {                                                                                                                   \
                V_CONCAT_VAR_NAME(vErrorDisplayed, __LINE__) = true;                                                            \
                V::Debug::Trace::Instance().Error(__FILE__, __LINE__, V_FUNCTION_SIGNATURE, window, __VA_ARGS__);               \
            }                                                                                                                   \
        }                                                                                                                       \
        V_POP_DISABLE_WARNING

    #define V_Warning(window, expression, ...)                                                                                  \
        V_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")                                                                \
        if (!(expression))                                                                                                      \
        {                                                                                                                       \
            V_TraceFmtCompileTimeCheck(expression, V_VA_HAS_ARGS(__VA_ARGS__),                                                  \
                "String used in place of boolean expression for V_Warning.",                                                    \
                "Did you mean V_Warning("#window", false, \"%s\", "#expression"); ?",                                           \
                "Did you mean V_Warning("#window", false, "#expression", "#__VA_ARGS__"); ?");                                  \
            V::Debug::Trace::Instance().Warning(__FILE__, __LINE__, V_FUNCTION_SIGNATURE, window, __VA_ARGS__);                 \
        }                                                                                                                       \
        V_POP_DISABLE_WARNING

    //! The V_WarningOnce macro output the result of the format string for once each use of the macro
    //! It does not take into account the result of the format string to determine whether to output the string or not
    //! What this means is that if the formatting results in different output string result only the first result
    //! will ever be output
    #define V_WarningOnce(window, expression, ...)                                                                              \
        V_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")                                                                \
        if (!(expression))                                                                                                      \
        {                                                                                                                       \
            V_TraceFmtCompileTimeCheck(expression, V_VA_HAS_ARGS(__VA_ARGS__),                                                  \
                "String used in place of boolean expression for V_WarningOnce.",                                                \
                "Did you mean V_WarningOnce("#window", false, \"%s\", "#expression"); ?",                                       \
                "Did you mean V_WarningOnce("#window", false, "#expression", "#__VA_ARGS__"); ?");                              \
            static bool V_CONCAT_VAR_NAME(vWarningDisplayed, __LINE__) = false;                                                 \
            if (!V_CONCAT_VAR_NAME(vWarningDisplayed, __LINE__))                                                                \
            {                                                                                                                   \
                V::Debug::Trace::Instance().Warning(__FILE__, __LINE__, V_FUNCTION_SIGNATURE, window, __VA_ARGS__);             \
                V_CONCAT_VAR_NAME(vWarningDisplayed, __LINE__) = true;                                                          \
            }                                                                                                                   \
        }                                                                                                                       \
        V_POP_DISABLE_WARNING

    #define V_TracePrintf(window, ...)                                                                                          \
        if(V::Debug::Trace::IsTraceLoggingEnabledForLevel(V::Debug::LogLevel::Info))                                            \
        {                                                                                                                       \
            V::Debug::Trace::Instance().Printf(window, __VA_ARGS__);                                                            \
        }


    //! The V_TrancePrintfOnce macro output the result of the format string only once for each use of the macro
    //! It does not take into account the result of the format string to determine whether to output the string or not
    //! What this means is that if the formatting results in different output string result only the first result
    //! will ever be output
    #define V_TracePrintfOnce(window, ...) \
        { \
            static bool V_CONCAT_VAR_NAME(vTracePrintfDisplayed, __LINE__) = false; \
            if (!V_CONCAT_VAR_NAME(vTracePrintfDisplayed, __LINE__)) \
            { \
                V_TracePrintf(window, __VA_ARGS__); \
                V_CONCAT_VAR_NAME(vTracePrintfDisplayed, __LINE__) = true; \
            } \
        }
    /*!
    * 验证跟踪检查的版本即使在发布中也会评估表达式.
    *
    * i.e.
    *   // with assert
    *   buffer = vmalloc(size,alignment);
    *   V_Assert( buffer != nullptr,"Assert Message");
    *   ...
    *
    *   // with verify
    *   V_Verify((buffer = vmalloc(size,alignment)) != nullptr,"Assert Message")
    *   ...
    */
    
    #define V_Verify(expression, ...) V_Assert(0 != (expression), __VA_ARGS__)
    #define V_VerifyError(window, expression, ...) V_Error(window, 0 != (expression), __VA_ARGS__)
    #define V_VerifyWarning(window, expression, ...) V_Warning(window, 0 != (expression), __VA_ARGS__)
#else // !V_ENABLE_TRACING
    #define V_Assert(...)
    #define V_Error(...)
    #define V_ErrorOnce(...)
    #define V_Warning(...)
    #define V_WarningOnce(...)
    #define V_TracePrintf(...)
    #define V_TracePrintfOnce(...)

    #define V_Verify(expression, ...)                  V_UNUSED(expression)
    #define V_VerifyError(window, expression, ...)     V_UNUSED(expression)
    #define V_VerifyWarning(window, expression, ...)   V_UNUSED(expression)

#endif // V_ENABLE_TRACING

#define V_Printf(window, ...)       V::Debug::Trace::Instance().Printf(window, __VA_ARGS__);

#if !defined(RELEASE)
// Unconditional critical error log, enabled up to Performance config
#define V_Fatal(window, format, ...)       V::Debug::Trace::Instance().Printf(window, "[FATAL] " format "\n", ##__VA_ARGS__);
#define V_Crash()                          V::Debug::Trace::Instance().Crash();
#else
#define V_Fatal(window, format, ...)
#define V_Crash()
#endif

#if defined(V_DEBUG_BUILD)
#   define V_DbgIf(expression) \
        V_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") \
        if (expression) \
        V_POP_DISABLE_WARNING
#   define V_DbgElseIf(expression) \
        V_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") \
        else if (expression) \
        V_POP_DISABLE_WARNING
#else
#   define V_DbgIf(expression)         if (false)
#   define V_DbgElseIf(expression)     else if (false)
#endif

#endif // V_FRAMEWORK_CORE_DEBUG_TRACE_H