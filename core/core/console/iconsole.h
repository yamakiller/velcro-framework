#ifndef V_FRAMEWORK_CORE_CONSOLE_ICONSOLE_H
#define V_FRAMEWORK_CORE_CONSOLE_ICONSOLE_H

#include <core/console/ConsoleFunctor.h>
#include <core/console/ConsoleDataWrapper.h>
#include <core/console/iconsole_types.h>
#include <core/event_bus/event.h>
#include <core/std/containers/vector.h>
#include <core/std/functional.h>

namespace V {
    class CommandLine;

    //! @class IConsole
    //! A simple console class for providing text based variable and process interaction.
    class IConsole
    {
    public:

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

#endif // V_FRAMEWORK_CORE_CONSOLE_ICONSOLE_H