#include <vcore/debug/trace.h>

#include <vcore/base.h>

#include <vcore/debug/stack_tracer.h>
#include <vcore/debug/ievent_logger.h>
#include <vcore/debug/trace_message_bus.h>
#include <vcore/debug/trace_message_detector_bus.h>
#include <vcore/interface/interface.h>
#include <vcore/math/crc.h>

#include <stdarg.h>

#include <vcore/std/containers/unordered_set.h>
#include <vcore/module/environment.h>
#include <vcore/console/iconsole.h>
#include <vcore/std/chrono/chrono.h>


namespace V 
{
    namespace Debug
    {
        namespace Platform
        {
#if defined(V_ENABLE_DEBUG_TOOLS)
            bool AttachDebugger();
            bool IsDebuggerPresent();
            void HandleExceptions(bool isEnabled);
            void DebugBreak();
#endif
            void Terminate(int exitCode);
        }
    }

    using namespace V::Debug;

    namespace DebugInternal
    {
        // other threads can trigger fatals and errors, but the same thread should not, to avoid stack overflow.
        // because its thread local, it does not need to be atomic.
        // its also important that this is inside the cpp file, so that its only in the trace.cpp module and not the header shared by everyone.
        static V_THREAD_LOCAL bool g_alreadyHandlingAssertOrFatal = false;
        V_THREAD_LOCAL bool g_suppressEBusCalls = false; // used when it would be dangerous to use ebus broadcasts.
    }

    //////////////////////////////////////////////////////////////////////////
    // Globals
    const int       g_maxMessageLength = 4096;
    static const char*    g_dbgSystemWnd = "System";
    Trace Debug::g_tracer;
    void* g_exceptionInfo = nullptr;

     // Environment var needed to track ignored asserts across systems and disable native UI under certain conditions
    static const char* ignoredAssertUID = "IgnoredAssertSet";
    static const char* assertVerbosityUID = "assertVerbosityLevel";
    static const char* logVerbosityUID = "sys_LogLevel";
    static const int assertLevel_log = 1;
    static const int assertLevel_nativeUI = 2;
    static const int assertLevel_crash = 3;
    static const int logLevel_full = 2;

    static V::EnvironmentVariable<VStd::unordered_set<size_t>> g_ignoredAsserts;
    static V::EnvironmentVariable<int> g_assertVerbosityLevel;
    static V::EnvironmentVariable<int> g_logVerbosityLevel;

    static constexpr auto PrintfEventId = EventNameHash("Printf");
    static constexpr auto WarningEventId = EventNameHash("Warning");
    static constexpr auto ErrorEventId = EventNameHash("Error");
    static constexpr auto AssertEventId = EventNameHash("Assert");

    constexpr LogLevel DefaultLogLevel = LogLevel::Info;

    V_CVAR_SCOPED(int, bg_traceLogLevel, DefaultLogLevel, nullptr, ConsoleFunctorFlags::Null, "Enable trace message logging in release mode.  0=disabled, 1=errors, 2=warnings, 3=info.");

     /**
     * If any listener returns true, store the result so we don't outputs detailed information.
     */
    struct TraceMessageResult
    {
        bool     m_value;
        TraceMessageResult()
            : m_value(false) {}
        void operator=(bool rhs)     { m_value = m_value || rhs; }
    };

    // definition of init to initialize assert tracking global
    void Trace::Init()
    {
        // if out ignored assert list exists, then another module has init the system
        g_ignoredAsserts = V::Environment::FindVariable<VStd::unordered_set<size_t>>(ignoredAssertUID);
        if (!g_ignoredAsserts)
        {
            g_ignoredAsserts = V::Environment::CreateVariable<VStd::unordered_set<size_t>>(ignoredAssertUID);
            g_assertVerbosityLevel = V::Environment::CreateVariable<int>(assertVerbosityUID);
            g_logVerbosityLevel = V::Environment::CreateVariable<int>(logVerbosityUID);

            //default assert level is to log/print asserts this can be overriden with the sys_asserts CVAR
            g_assertVerbosityLevel.Set(assertLevel_log);
            g_logVerbosityLevel.Set(logLevel_full);
        }
    }

     // clean up the ignored assert container
    void Trace::Destroy()
    {
        g_ignoredAsserts = V::Environment::FindVariable<VStd::unordered_set<size_t>>(ignoredAssertUID);
        if (g_ignoredAsserts)
        {
            if (g_ignoredAsserts.IsOwner())
            {
                g_ignoredAsserts.Reset();
            }
        }
    }

    //=========================================================================
    const char* Trace::GetDefaultSystemWindow()
    {
        return g_dbgSystemWnd;
    }

    //=========================================================================
    // IsDebuggerPresent
    //=========================================================================
    bool
    Trace::IsDebuggerPresent()
    {
#if defined(V_ENABLE_DEBUG_TOOLS)
        return Platform::IsDebuggerPresent();
#else
        return false;
#endif
    }

    bool
    Trace::AttachDebugger()
    {
#if defined(V_ENABLE_DEBUG_TOOLS)
        return Platform::AttachDebugger();
#else
        return false;
#endif
    }

    bool Trace::WaitForDebugger([[maybe_unused]] float timeoutSeconds/*=-1.f*/)
    {
#if defined(V_ENABLE_DEBUG_TOOLS)
        using VStd::chrono::system_clock;
        using VStd::chrono::time_point;
        using VStd::chrono::milliseconds;

        milliseconds timeoutMs = milliseconds(vnumeric_cast<long long>(timeoutSeconds * 1000));
        system_clock clock;
        time_point start = clock.now();
        auto hasTimedOut = [&clock, start, timeoutMs]()
        {
            return timeoutMs.count() >= 0 && (clock.now() - start) >= timeoutMs;
        };

        while (!V::Debug::Trace::IsDebuggerPresent() && !hasTimedOut())
        {
            VStd::this_thread::sleep_for(milliseconds(1));
        }
        return V::Debug::Trace::IsDebuggerPresent();
#else
        return false;
#endif
    }

    //=========================================================================
    // HandleExceptions
    //=========================================================================
    void Trace::HandleExceptions(bool isEnabled)
    {
        V_UNUSED(isEnabled);
        if (IsDebuggerPresent())
        {
            return;
        }

#if defined(V_ENABLE_DEBUG_TOOLS)
        Platform::HandleExceptions(isEnabled);
#endif
    }

    //=========================================================================
    // Break
    //=========================================================================
    void
    Trace::Break()
    {
#if defined(V_ENABLE_DEBUG_TOOLS)

        if (!IsDebuggerPresent())
        {
            return; // Do not break when tests are running unless debugger is present
        }

        Platform::DebugBreak();

#endif // V_ENABLE_DEBUG_TOOLS
    }

    void Debug::Trace::Crash()
    {
        int* p = nullptr;
        *p = 1;
    }

    void Debug::Trace::Terminate(int exitCode)
    {
        Platform::Terminate(exitCode);
    }

    bool Trace::IsTraceLoggingEnabledForLevel(LogLevel level)
    {
        return bg_traceLogLevel >= level;
    }

    //=========================================================================
    // Error
    //=========================================================================
    void Trace::Error(const char* fileName, int line, const char* funcName, const char* window, const char* format, ...)
    {
        if (!IsTraceLoggingEnabledForLevel(LogLevel::Errors))
        {
            return;
        }

        using namespace DebugInternal;
        if (!window)
        {
            window = g_dbgSystemWnd;
        }

        char message[g_maxMessageLength];
        char header[g_maxMessageLength];

        if (g_alreadyHandlingAssertOrFatal)
        {
            return;
        }
        g_alreadyHandlingAssertOrFatal = true;

        va_list mark;
        va_start(mark, format);
        vvsnprintf(message, g_maxMessageLength-1, format, mark); // -1 to make room for the "/n" that will be appended below 
        va_end(mark);

        if (auto logger = Interface<IEventLogger>::Get("IEventLogger"); logger)
        {
            logger->RecordStringEvent(ErrorEventId, message);
        }

        EBUS_EVENT(TraceMessageDetectorBus, OnPreError, window, fileName, line, funcName, message);

        TraceMessageResult result;
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnPreError, window, fileName, line, funcName, message);
        if (result.m_value)
        {
            g_alreadyHandlingAssertOrFatal = false;
            return;
        }

        Output(window, "\n==================================================================\n");
        vsnprintfv(header, g_maxMessageLength, "Trace::Error\n %s(%d): '%s'\n", fileName, line, funcName);
        Output(window, header);
        vstrcat(message, g_maxMessageLength, "\n");
        Output(window, message);

        EBUS_EVENT(TraceMessageDetectorBus, OnError, window, message);
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnError, window, message);
        Output(window, "==================================================================\n");
        if (result.m_value)
        {
            g_alreadyHandlingAssertOrFatal = false;
            return;
        }

        g_alreadyHandlingAssertOrFatal = false;
    }
    //=========================================================================
    // Warning
    //=========================================================================
    void
    Trace::Warning(const char* fileName, int line, const char* funcName, const char* window, const char* format, ...)
    {
        if (!IsTraceLoggingEnabledForLevel(LogLevel::Warnings))
        {
            return;
        }

        char message[g_maxMessageLength];
        char header[g_maxMessageLength];

        va_list mark;
        va_start(mark, format);
        vvsnprintf(message, g_maxMessageLength - 1, format, mark); // -1 to make room for the "/n" that will be appended below 
        va_end(mark);

        if (auto logger = Interface<IEventLogger>::Get(); logger)
        {
            logger->RecordStringEvent(WarningEventId, message);
        }

        EBUS_EVENT(TraceMessageDetectorBus, OnPreWarning, window, fileName, line, funcName, message);

        TraceMessageResult result;
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnPreWarning, window, fileName, line, funcName, message);
        if (result.m_value)
        {
            return;
        }

        Output(window, "\n==================================================================\n");
        vsnprintfv(header, g_maxMessageLength, "Trace::Warning\n %s(%d): '%s'\n", fileName, line, funcName);
        Output(window, header);
        vstrcat(message, g_maxMessageLength, "\n");
        Output(window, message);

        EBUS_EVENT(TraceMessageDetectorBus, OnWarning, window, message);
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnWarning, window, message);
        Output(window, "==================================================================\n");
    }

    //=========================================================================
    // Printf
    //=========================================================================
    void
    Trace::Printf(const char* window, const char* format, ...)
    {
        if (!window)
        {
            window = g_dbgSystemWnd;
        }

        char message[g_maxMessageLength];

        va_list mark;
        va_start(mark, format);
        vvsnprintf(message, g_maxMessageLength, format, mark);
        va_end(mark);

        if (auto logger = Interface<IEventLogger>::Get("IEventLogger"); logger)
        {
            logger->RecordStringEvent(PrintfEventId, message);
        }

        EBUS_EVENT(TraceMessageDetectorBus, OnPrintf, window, message);

        TraceMessageResult result;
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnPrintf, window, message);
        if (result.m_value)
        {
            return;
        }

        Output(window, message);
    }

    //=========================================================================
    // Output
    //=========================================================================
    void Trace::Output(const char* window, const char* message)
    {
        if (!window)
        {
            window = g_dbgSystemWnd;
        }

        Platform::OutputToDebugger(window, message);
        
        if (!DebugInternal::g_suppressEBusCalls)
        {
            // only call into Ebusses if we are not in a recursive-exception situation as that
            // would likely just lead to even more exceptions.
            
            EBUS_EVENT(TraceMessageDetectorBus, OnOutput, window, message);
            TraceMessageResult result;
            EBUS_EVENT_RESULT(result, TraceMessageBus, OnOutput, window, message);
            if (result.m_value)
            {
                return;
            }
        }

        // printf on Windows platforms seem to have a buffer length limit of 4096 characters
        // Therefore fwrite is used directly to write the window and message to stdout
        VStd::string_view windowView{ window };
        VStd::string_view messageView{ message };
        constexpr VStd::string_view windowMessageSeparator{ ": " };
        fwrite(windowView.data(), 1, windowView.size(), stdout);
        fwrite(windowMessageSeparator.data(), 1, windowMessageSeparator.size(), stdout);
        fwrite(messageView.data(), 1, messageView.size(), stdout);
    }

    //=========================================================================
    // PrintCallstack
    //=========================================================================
    void Trace::PrintCallstack(const char* window, unsigned int suppressCount, void* nativeContext)
    {
        StackFrame frames[25];

        // Without StackFrame explicit alignment frames array is aligned to 4 bytes
        // which causes the stack tracing to fail.
        //size_t bla = VStd::alignment_of<StackFrame>::value;
        //printf("Alignment value %d address 0x%08x : 0x%08x\n",bla,frames);
        SymbolStorage::StackLine lines[V_ARRAY_SIZE(frames)];

        if (!nativeContext)
        {
            suppressCount += 1; /// If we don't provide a context we will capture in the RecordFunction, so skip us (Trace::PrinCallstack).
        }
        unsigned int numFrames = StackRecorder::Record(frames, V_ARRAY_SIZE(frames), suppressCount, nativeContext);
        if (numFrames)
        {
            SymbolStorage::DecodeFrames(frames, numFrames, lines);
            for (unsigned int i = 0; i < numFrames; ++i)
            {
                if (lines[i][0] == 0)
                {
                    continue;
                }

                vstrcat(lines[i], V_ARRAY_SIZE(lines[i]), "\n");
                V_Printf(window, "%s", lines[i]); // feed back into the trace system so that listeners can get it.
            }
        }
    }

    //=========================================================================
    // GetExceptionInfo
    //=========================================================================
    void* Trace::GetNativeExceptionInfo()
    {
        return g_exceptionInfo;
    }

    // Gets/sets the current verbosity level from the global
    int Trace::GetAssertVerbosityLevel()
    {
        auto val = V::Environment::FindVariable<int>(assertVerbosityUID);
        if (val)
        {
            return val.Get();
        }
        else
        {
            return assertLevel_log;
        }
    }

    void Trace::SetAssertVerbosityLevel(int level)
    {
        auto val = V::Environment::FindVariable<int>(assertVerbosityUID);
        if (val)
        {
            val.Set(level);
        }
    }
}