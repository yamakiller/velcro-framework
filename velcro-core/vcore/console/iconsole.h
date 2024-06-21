#ifndef V_FRAMEWORK_CORE_CONSOLE_ICONSOLE_H
#define V_FRAMEWORK_CORE_CONSOLE_ICONSOLE_H

#include <vcore/console/console_functor.h>
#include <vcore/console/console_data_wrapper.h>
#include <vcore/console/iconsole_types.h>
#include <vcore/event_bus/event.h>
#include <vcore/std/containers/vector.h>
#include <vcore/std/functional.h>

namespace V {
    class CommandLine;

    //! @class IConsole
    //! A simple console class for providing text based variable and process interaction.
    class IConsole
    {
    public:
        VELCRO_TYPE_INFO(IConsole, "{77fbb393-05ef-4b9c-85bd-80279e7199e1}");
        
        using FunctorVisitor = VStd::function<void(ConsoleFunctorBase*)>;

        inline static constexpr VStd::string_view ConsoleRootCommandKey = "/Velcro/Core/Runtime/ConsoleCommands";

        IConsole() = default;
        virtual ~IConsole() = default;

        //! Invokes a single console command, optionally returning the command output.
        //! @param command       the command string to parse and execute on
        //! @param silentMode    if true, logs will be suppressed during command execution
        //! @param invokedFrom   the source point that initiated console invocation
        //! @param requiredSet   a set of flags that must be set on the functor for it to execute
        //! @param requiredClear a set of flags that must *NOT* be set on the functor for it to execute
        //! @return boolean true on success, false otherwise
        virtual bool PerformCommand
        (
            const char* command,
            ConsoleSilentMode silentMode = ConsoleSilentMode::NotSilent,
            ConsoleInvokedFrom invokedFrom = ConsoleInvokedFrom::VelcroConsole,
            ConsoleFunctorFlags requiredSet = ConsoleFunctorFlags::Null,
            ConsoleFunctorFlags requiredClear = ConsoleFunctorFlags::ReadOnly
        ) = 0;

        //! Invokes a single console command, optionally returning the command output.
        //! @param commandAndArgs list of command and command arguments to execute. The first argument is the command itself
        //! @param silentMode     if true, logs will be suppressed during command execution
        //! @param invokedFrom    the source point that initiated console invocation
        //! @param requiredSet    a set of flags that must be set on the functor for it to execute
        //! @param requiredClear  a set of flags that must *NOT* be set on the functor for it to execute
        //! @return boolean true on success, false otherwise
        virtual bool PerformCommand
        (
            const ConsoleCommandContainer& commandAndArgs,
            ConsoleSilentMode silentMode = ConsoleSilentMode::NotSilent,
            ConsoleInvokedFrom invokedFrom = ConsoleInvokedFrom::VelcroConsole,
            ConsoleFunctorFlags requiredSet = ConsoleFunctorFlags::Null,
            ConsoleFunctorFlags requiredClear = ConsoleFunctorFlags::ReadOnly
        ) = 0;

        //! Invokes a single console command, optionally returning the command output.
        //! @param command       the command to execute
        //! @param commandArgs   the arguments to the command to execute
        //! @param silentMode    if true, logs will be suppressed during command execution
        //! @param invokedFrom   the source point that initiated console invocation
        //! @param requiredSet   a set of flags that must be set on the functor for it to execute
        //! @param requiredClear a set of flags that must *NOT* be set on the functor for it to execute
        //! @return boolean true on success, false otherwise
        virtual bool PerformCommand
        (
            VStd::string_view command,
            const ConsoleCommandContainer& commandArgs,
            ConsoleSilentMode silentMode = ConsoleSilentMode::NotSilent,
            ConsoleInvokedFrom invokedFrom = ConsoleInvokedFrom::VelcroConsole,
            ConsoleFunctorFlags requiredSet = ConsoleFunctorFlags::Null,
            ConsoleFunctorFlags requiredClear = ConsoleFunctorFlags::ReadOnly
        ) = 0;


        //! Loads and executes the specified config file.
        //! @param configFileName the filename of the config file to load and execute
        virtual void ExecuteConfigFile(VStd::string_view configFileName) = 0;

        //! Invokes all of the commands as contained in a concatenated command-line string.
        //! @param commandLine the concatenated command-line string to execute
        virtual void ExecuteCommandLine(const V::CommandLine& commandLine) = 0;

        //! Attempts to invoke a "deferred console command", which is a console command
        //! that has failed to execute previously due to the command not being registered yet.
        //! @return boolean true if any deferred console commands have executed, false otherwise
        virtual bool ExecuteDeferredConsoleCommands() = 0;

        //! Clear out any deferred console commands queue
        virtual void ClearDeferredConsoleCommands() = 0;

        //! HasCommand is used to determine if the console knows about a command.
        //! @param command the command we are checking for
        //! @return boolean true on if the command is registered, false otherwise
        virtual bool IsCommand(VStd::string_view command) = 0;

        //! FindCommand finds the console command with the specified console string.
        //! @param command the command that is being searched for
        //! @return non-null pointer to the console command if found
        virtual ConsoleFunctorBase* FindCommand(VStd::string_view command) = 0;

        //! Finds all commands where the input command is a prefix and returns
        //! the longest matching substring prefix the results have in common.
        //! @param command The prefix string to find all matching commands for.
        //! @param matches The list of all commands that match the input prefix.
        //! @return The longest matching substring prefix the results have in common.
        virtual VStd::string AutoCompleteCommand(VStd::string_view command,
                                                 VStd::vector<VStd::string>* matches = nullptr) = 0;

        //! Retrieves the value of the requested cvar.
        //! @param command the name of the cvar to find and retrieve the current value of
        //! @param outValue reference to the instance to write the current cvar value to
        //! @return GetValueResult::Success if the operation succeeded, or an error result if the operation failed
        template<typename RETURN_TYPE>
        GetValueResult GetCvarValue(VStd::string_view command, RETURN_TYPE& outValue);

        //! Visits all registered console functors.
        //! @param visitor the instance to visit all functors with
        virtual void VisitRegisteredFunctors(const FunctorVisitor& visitor) = 0;

        //! Registers a ConsoleFunctor with the console instance.
        //! @param functor pointer to the ConsoleFunctor to register
        virtual void RegisterFunctor(ConsoleFunctorBase* functor) = 0;

        //! Unregisters a ConsoleFunctor with the console instance.
        //! @param functor pointer to the ConsoleFunctor to unregister
        virtual void UnregisterFunctor(ConsoleFunctorBase* functor) = 0;

        //! Should be invoked for every module that gets loaded.
        //! @param pointer to the modules set of ConsoleFunctors to register
        virtual void LinkDeferredFunctors(ConsoleFunctorBase*& deferredHead) = 0;

        //! Returns the V::Event<> invoked whenever a console command is registered.
        using ConsoleCommandRegisteredEvent = V::Event<ConsoleFunctorBase*>;
        ConsoleCommandRegisteredEvent& GetConsoleCommandRegisteredEvent();

        //! Returns the V::Event<> invoked whenever a console command is executed.
        ConsoleCommandInvokedEvent& GetConsoleCommandInvokedEvent();

        //! Returns the V::Event<> invoked whenever a console command could not be found.
        DispatchCommandNotFoundEvent& GetDispatchCommandNotFoundEvent();

        V_DISABLE_COPY_MOVE(IConsole);
    protected:
        ConsoleCommandRegisteredEvent m_consoleCommandRegisteredEvent;
        ConsoleCommandInvokedEvent m_consoleCommandInvokedEvent;
        DispatchCommandNotFoundEvent m_dispatchCommandNotFoundEvent;
    };

    inline auto IConsole::GetConsoleCommandRegisteredEvent() -> ConsoleCommandRegisteredEvent&
    {
        return m_consoleCommandRegisteredEvent;
    }

    inline auto IConsole::GetConsoleCommandInvokedEvent() -> ConsoleCommandInvokedEvent&
    {
        return m_consoleCommandInvokedEvent;
    }

    inline auto IConsole::GetDispatchCommandNotFoundEvent() -> DispatchCommandNotFoundEvent&
    {
        return m_dispatchCommandNotFoundEvent;
    }

    template<typename RETURN_TYPE>
    inline GetValueResult IConsole::GetCvarValue(VStd::string_view command, RETURN_TYPE& outValue)
    {
        ConsoleFunctorBase* cvarFunctor = FindCommand(command);
        if (cvarFunctor == nullptr)
        {
            return GetValueResult::ConsoleVarNotFound;
        }
        return cvarFunctor->GetValue(outValue);
    }
}

template <typename _TYPE, typename = void>
static constexpr V::ThreadSafety ConsoleThreadSafety = V::ThreadSafety::RequiresLock;

template <typename _TYPE>
static constexpr V::ThreadSafety ConsoleThreadSafety<_TYPE, std::enable_if_t<std::is_arithmetic_v<_TYPE>>> = V::ThreadSafety::UseStdAtomic;

//! Standard cvar macro.
//! @param _TYPE the data type of the cvar
//! @param _NAME the name of the cvar
//! @param _INIT the initial value to assign to the cvar
//! @param _CALLBACK this is an optional callback function to get invoked when a cvar changes value
//!        You have no guarantees as to what thread will invoke the callback
//!        It is the responsibility of the implementor of the callback to ensure thread safety
//! @param _FLAGS a set of VelcroFramework::ConsoleFunctorFlags used to mutate behaviour
//! @param _DESC a description of the cvar
#define V_CVAR(_TYPE, _NAME, _INIT, _CALLBACK, _FLAGS, _DESC) \
    using CVarDataWrapperType##_NAME = V::ConsoleDataWrapper<_TYPE, ConsoleThreadSafety<_TYPE>>; \
    inline CVarDataWrapperType##_NAME _NAME(_INIT, _CALLBACK, #_NAME, _DESC, _FLAGS)

//! Block-scoped cvar macro.
//! This declaration should only be used within a block-scope or function body
//! @param _TYPE the data type of the cvar
//! @param _NAME the name of the cvar
//! @param _INIT the initial value to assign to the cvar
//! @param _CALLBACK this is an optional callback function to get invoked when a cvar changes value
//! @param _FLAGS a set of VelcroFramework::ConsoleFunctorFlags used to mutate behaviour
//!        You have no guarantees as to what thread will invoke the callback
//!        It is the responsibility of the implementor of the callback to ensure thread safety
//! @param _DESC a description of the cvar
#define V_CVAR_SCOPED(_TYPE, _NAME, _INIT, _CALLBACK, _FLAGS, _DESC) \
    using CVarDataWrapperType##_NAME = V::ConsoleDataWrapper<_TYPE, ConsoleThreadSafety<_TYPE>>; \
    CVarDataWrapperType##_NAME _NAME(_INIT, _CALLBACK, #_NAME, _DESC, _FLAGS)

//! Cvar macro that externs a console variable.
//! @param _TYPE the data type of the cvar to extern
//! @param _NAME the name of the cvar to extern
#define V_CVAR_EXTERNED(_TYPE, _NAME) \
    using CVarDataWrapperType##_NAME = V::ConsoleDataWrapper<_TYPE, ConsoleThreadSafety<_TYPE>>; \
    extern CVarDataWrapperType##_NAME _NAME;

//! Implements a console functor for a class member function.
//! @param _CLASS the that the function gets invoked on
//! @param _FUNCTION the method to invoke
//!        You have no guarantees as to what thread will invoke the function
//!        It is the responsibility of the implementor of the console function to ensure thread safety
//! @param _FLAGS a set of VelcroFramework::ConsoleFunctorFlags used to mutate behaviour
//! @param _DESC a description of the cvar
#define V_CONSOLEFUNC(_CLASS, _FUNCTION, _FLAGS, _DESC) \
    V::ConsoleFunctor<_CLASS, false> m_functor##_FUNCTION{#_CLASS "." #_FUNCTION, _DESC, _FLAGS, V::TypeId::CreateNull(), *this, &_CLASS::_FUNCTION}

//! Implements a console functor for a non-member function.
//! @param _FUNCTION the method to invoke
//!        ** YOU HAVE NO GUARANTEES AS TO WHAT THREAD WILL INVOKE YOUR FUNCTION ** It is the responsibility of the implementor of the console function to ensure thread safety
//! @param _FLAGS a set of VelcroFramework::ConsoleFunctorFlags used to mutate behaviour
//! @param _DESC a description of the cvar
#define V_CONSOLEFREEFUNC_3(_FUNCTION, _FLAGS, _DESC) \
    inline V::ConsoleFunctor<void, false> Functor##_FUNCTION(#_FUNCTION, _DESC, _FLAGS | V::ConsoleFunctorFlags::DontDuplicate, V::TypeId::CreateNull(), &_FUNCTION)

//! Implements a console functor for a non-member function.
//!
//! @param _FUNCTION the method to invoke
//!        ** YOU HAVE NO GUARANTEES AS TO WHAT THREAD WILL INVOKE YOUR FUNCTION ** It is the responsibility of the implementor of the console function to ensure thread safety
//! @param _NAME the name the of the function in the console
//! @param _FLAGS a set of VelcroFramework::ConsoleFunctorFlags used to mutate behaviour
//! @param _DESC a description of the cvar
#define V_CONSOLEFREEFUNC_4(_NAME, _FUNCTION, _FLAGS, _DESC) \
    inline V::ConsoleFunctor<void, false> Functor##_FUNCTION(#_FUNCTION, _DESC, _FLAGS | V::ConsoleFunctorFlags::DontDuplicate, V::TypeId::CreateNull(), &_FUNCTION)

#define V_CONSOLEFREEFUNC(...) V_MACRO_SPECIALIZE(V_CONSOLEFREEFUNC_, V_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

#endif // V_FRAMEWORK_CORE_CONSOLE_ICONSOLE_H