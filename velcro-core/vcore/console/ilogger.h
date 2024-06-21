#ifndef V_FRAMEWORK_CORE_CONSOLE_ILOGGER_H
#define V_FRAMEWORK_CORE_CONSOLE_ILOGGER_H

#include <vcore/interface/interface.h>
#include <vcore/utilitys/utility.h>
#include <vcore/event_bus/event_bus.h>
#include <vcore/event_bus/event.h>
#include <stdarg.h>

namespace V {
    //! This essentially maps to standard syslog logging priorities to allow the logger to easily plug into standard logging services
    enum class LogLevel : int8_t { Trace = 1, Debug = 2, Info = 3, Notice = 4, Warn = 5, Error = 6, Fatal = 7 };

    //! @class ILogger
    //! @brief This is an V::Interface<> for logging.
    //! Usage:
    //!  #include <vcore/console/ilogger.h>
    //!  VLOG_INFO("Your message here");
    //!  VLOG_WARN("Your warn message here");
    class ILogger
    {
    public:
        VELCRO_TYPE_INFO(ILogger, "{e5f5d1f2-fd9b-416b-9380-9e29e11910a1}");
        
        // LogLevel, message, file, function, line
        using LogEvent = V::Event<LogLevel, const char*, const char*, const char*, int32_t>;

        ILogger() = default;
        virtual ~ILogger() = default;

        //! Sets the the name of the log file.
        //! @param a_LogName the new logfile name to use
        virtual void SetLogName(const char* logName) = 0;

        //! Gets the the name of the log file.
        //! @return the current logfile name
        virtual const char* GetLogName() const = 0;

        //! Sets the log level for the logger instance.
        //! @param logLevel the minimum log level to filter out log messages at
        virtual void SetLogLevel(LogLevel logLevel) = 0;

        //! Gets the log level for the logger instance.
        //! @return the current minimum log level to filter out log messages at
        virtual LogLevel GetLogLevel() const = 0;

        //! Binds a log event handler.
        //! @param handler the handler to bind to logging events
        virtual void BindLogHandler(LogEvent::Handler& hander) = 0;

        //! Queries whether the provided logging tag is enabled.
        //! @param hashValue the hash value for the provided logging tag
        //! @return boolean true if enabled
        virtual bool IsTagEnabled(V::HashValue32 hashValue) = 0;

        //! Immediately Flushes any pending messages without waiting for next thread update.
        //! Should be invoked whenever unloading any shared library or module to avoid crashing on dangling string pointers
        virtual void Flush() = 0;

        //! Don't use this directly, use the logger macros defined below (VLOG_INFO, VLOG_WARN, VLOG_ERROR, etc..)
        virtual void LogInternalV(LogLevel level, const char* format, const char* file, const char* function, int32_t line, va_list args) = 0;

        //! Don't use this directly, use the logger macros defined below (VLOG_INFO, VLOG_WARN, VLOG_ERROR, etc..)
        inline void LogInternal(LogLevel level, const char* format, const char* file, const char* function, int32_t line, ...) V_FORMAT_ATTRIBUTE(3, 7)
        {
            va_list args;
            va_start(args, line);
            LogInternalV(level, format, file, function, line, args);
            va_end(args);
        }
    };
}

#define VELCROLOG_TRACE(MESSAGE, ...)                                                                        \
{                                                                                                            \
    V::ILogger* logger = V::Interface<V::ILogger>::Get();                                                    \
    if (logger != nullptr && V::LogLevel::Trace >= logger->GetLogLevel())                                    \
    {                                                                                                        \
        logger->LogInternal(V::LogLevel::Trace, MESSAGE, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);   \
    }                                                                                                        \
}

#define VELCROLOG_DEBUG(MESSAGE, ...)                                                                        \
{                                                                                                            \
    V::ILogger* logger = V::Interface<V::ILogger>::Get();                                                    \
    if (logger != nullptr && V::LogLevel::Debug >= logger->GetLogLevel())                                    \
    {                                                                                                        \
        logger->LogInternal(V::LogLevel::Debug, MESSAGE, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);   \
    }                                                                                                        \
}

#define VELCROLOG_INFO(MESSAGE, ...)                                                                         \
{                                                                                                            \
    V::ILogger* logger = V::Interface<V::ILogger>::Get();                                                    \
    if (logger != nullptr && V::LogLevel::Info >= logger->GetLogLevel())                                     \
    {                                                                                                        \
        logger->LogInternal(V::LogLevel::Info, MESSAGE, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);    \
    }                                                                                                        \
}

#define VELCROLOG_NOTICE(MESSAGE, ...)                                                                       \
{                                                                                                            \
    V::ILogger* logger = V::Interface<V::ILogger>::Get();                                                    \
    if (logger != nullptr && V::LogLevel::Notice >= logger->GetLogLevel())                                   \
    {                                                                                                        \
        logger->LogInternal(V::LogLevel::Notice, MESSAGE, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
    }                                                                                                        \
}

#define VELCROLOG_WARN(MESSAGE, ...)                                                                         \
{                                                                                                            \
    V::ILogger* logger = V::Interface<V::ILogger>::Get();                                                    \
    if (logger != nullptr && V::LogLevel::Warn >= logger->GetLogLevel())                                     \
    {                                                                                                        \
        logger->LogInternal(V::LogLevel::Warn, MESSAGE, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);    \
    }                                                                                                        \
}

#define VELCROLOG_ERROR(MESSAGE, ...)                                                                        \
{                                                                                                            \
    V::ILogger* logger = V::Interface<V::ILogger>::Get();                                                    \
    if (logger != nullptr && V::LogLevel::Error >= logger->GetLogLevel())                                    \
    {                                                                                                        \
        logger->LogInternal(V::LogLevel::Error, MESSAGE, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);   \
    }                                                                                                        \
}

#define VELCROLOG_FATAL(MESSAGE, ...)                                                                        \
{                                                                                                            \
    V::ILogger* logger = V::Interface<V::ILogger>::Get();                                                    \
    if (logger != nullptr && V::LogLevel::Fatal >= logger->GetLogLevel())                                    \
    {                                                                                                        \
        logger->LogInternal(V::LogLevel::Fatal, MESSAGE, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);   \
    }                                                                                                        \
}

#define VELCROLOG(TAG, MESSAGE, ...)                                                                         \
{                                                                                                            \
    static const V::HashValue32 hashValue = V::TypeHash32(#TAG);                                             \
    V::ILogger* logger = V::Interface<V::ILogger>::Get();                                                    \
    if (logger != nullptr && logger->IsTagEnabled(hashValue))                                                \
    {                                                                                                        \
        logger->LogInternal(V::LogLevel::Notice, MESSAGE, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
    }                                                                                                        \
}

#define VELCROLOG_FLUSH()                                                                                    \
{                                                                                                            \
    V::ILogger* logger = V::Interface<V::ILogger>::Get();                                                    \
    if (logger != nullptr)                                                                                   \
    {                                                                                                        \
        logger->Flush();                                                                                     \
    }                                                                                                        \
}


#endif // V_FRAMEWORK_CORE_CONSOLE_ILOGGER_H