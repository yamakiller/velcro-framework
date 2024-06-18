#ifndef V_FRAMEWORK_CORE_CONSOLE_ILOGGER_H
#define V_FRAMEWORK_CORE_CONSOLE_ILOGGER_H

#include <core/interface/interface.h>
#include <core/utilitys/utility.h>
#include <core/event_bus/event_bus.h>
#include <core/event_bus/event.h>
#include <stdarg.h>

namespace V {
    //! This essentially maps to standard syslog logging priorities to allow the logger to easily plug into standard logging services
    enum class LogLevel : int8_t { Trace = 1, Debug = 2, Info = 3, Notice = 4, Warn = 5, Error = 6, Fatal = 7 };

    //! @class ILogger
    //! @brief This is an V::Interface<> for logging.
    //! Usage:
    //!  #include <core/console/ilogger.h>
    //!  VLOG_INFO("Your message here");
    //!  VLOG_WARN("Your warn message here");
    class ILogger
    {
    public:

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

#endif // V_FRAMEWORK_CORE_CONSOLE_ILOGGER_H