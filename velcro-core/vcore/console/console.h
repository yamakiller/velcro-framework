#ifndef V_FRAMEWORK_CORE_CONSOLE_CONSOLE_H
#define V_FRAMEWORK_CORE_CONSOLE_CONSOLE_H

#include <vcore/console/iconsole.h>
#include <vcore/Memory/osallocator.h>
#include <vcore/std/functional.h>
#include <vcore/std/containers/unordered_map.h>

namespace V
{
    //! @class Console
    //! A simple console class for providing text based variable and process interaction.
    class Console final : public IConsole
    {
        friend struct ConsoleCommandKeyNotificationHandler;
        friend class ConsoleFunctorBase;
    public:
        VELCRO_TYPE_INFO(Console, "{a501459d-a922-46d3-ad6d-d14aefbc4774}");
        V_CLASS_ALLOCATOR(Console, V::OSAllocator, 0);

        Console();
      
        ~Console() override;

        //! IConsole interface
        //! @{
        bool PerformCommand
        (
            const char* command,
            ConsoleSilentMode silentMode = ConsoleSilentMode::NotSilent,
            ConsoleInvokedFrom invokedFrom = ConsoleInvokedFrom::VelcroConsole,
            ConsoleFunctorFlags requiredSet = ConsoleFunctorFlags::Null,
            ConsoleFunctorFlags requiredClear = ConsoleFunctorFlags::ReadOnly
        ) override;
        bool PerformCommand
        (
            const ConsoleCommandContainer& commandAndArgs,
            ConsoleSilentMode silentMode = ConsoleSilentMode::NotSilent,
            ConsoleInvokedFrom invokedFrom = ConsoleInvokedFrom::VelcroConsole,
            ConsoleFunctorFlags requiredSet = ConsoleFunctorFlags::Null,
            ConsoleFunctorFlags requiredClear = ConsoleFunctorFlags::ReadOnly
        ) override;
        bool PerformCommand
        (
            VStd::string_view command,
            const ConsoleCommandContainer& commandArgs,
            ConsoleSilentMode silentMode = ConsoleSilentMode::NotSilent,
            ConsoleInvokedFrom invokedFrom = ConsoleInvokedFrom::VelcroConsole,
            ConsoleFunctorFlags requiredSet = ConsoleFunctorFlags::Null,
            ConsoleFunctorFlags requiredClear = ConsoleFunctorFlags::ReadOnly
        ) override;
        void ExecuteConfigFile(VStd::string_view configFileName) override;
        void ExecuteCommandLine(const V::CommandLine& commandLine) override;
        bool ExecuteDeferredConsoleCommands() override;

        void ClearDeferredConsoleCommands() override;

        bool IsCommand(VStd::string_view command) override;
        ConsoleFunctorBase* FindCommand(VStd::string_view command) override;
        VStd::string AutoCompleteCommand(VStd::string_view command, VStd::vector<VStd::string>* matches = nullptr) override;
        void VisitRegisteredFunctors(const FunctorVisitor& visitor) override;
        void RegisterFunctor(ConsoleFunctorBase* functor) override;
        void UnregisterFunctor(ConsoleFunctorBase* functor) override;
        void LinkDeferredFunctors(ConsoleFunctorBase*& deferredHead) override;

        //! @}

    private:

        void MoveFunctorsToDeferredHead(ConsoleFunctorBase*& deferredHead);

        //! Invokes a single console command, optionally returning the command output.
        //! @param command       the function to invoke
        //! @param inputs        the set of inputs to provide the function
        //! @param silentMode    if true, logs will be suppressed during command execution
        //! @param invokedFrom   the source point that initiated console invocation
        //! @param requiredSet   a set of flags that must be set on the functor for it to execute
        //! @param requiredClear a set of flags that must *NOT* be set on the functor for it to execute
        //! @return boolean true on success, false otherwise
        bool DispatchCommand
        (
            VStd::string_view command,
            const ConsoleCommandContainer& inputs,
            ConsoleSilentMode silentMode,
            ConsoleInvokedFrom invokedFrom,
            ConsoleFunctorFlags requiredSet,
            ConsoleFunctorFlags requiredClear
        );

        V_DISABLE_COPY_MOVE(Console);

       
        using CommandMap = VStd::unordered_map<CVarFixedString, VStd::vector<ConsoleFunctorBase*>>;
        struct DeferredCommand
        {
            using DeferredArguments = VStd::vector<VStd::string>;
            VStd::string m_command;
            DeferredArguments m_arguments;
            ConsoleSilentMode m_silentMode;
            ConsoleInvokedFrom m_invokedFrom;
            ConsoleFunctorFlags m_requiredSet;
            ConsoleFunctorFlags m_requiredClear;
        };
        using DeferredCommandQueue = VStd::deque<DeferredCommand>;
    private:
        ConsoleFunctorBase* m_head;
        CommandMap m_commands;
        DeferredCommandQueue m_deferredCommands;
    };
}


#endif // V_FRAMEWORK_CORE_CONSOLE_CONSOLE_H