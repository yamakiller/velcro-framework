#include <vcore/console/console.h>
#include <vcore/console/ILogger.h>
#include <vcore/interface/interface.h>
#include <vcore/settings/command_line.h>
#include <vcore/string_func/string_func.h>
#include <vcore/utilitys/utility.h>
#include <vcore/io/file_io.h>
#include <vcore/io/path/path.h>
#include <vcore/io/system_file.h>
#include <cctype>

namespace V
{
    uint32_t CountMatchingPrefixes(const VStd::string_view& string, const ConsoleCommandContainer& stringSet)
    {
        uint32_t count = 0;

        for (VStd::string_view iter : stringSet)
        {
            if (StringFunc::StartsWith(iter, string, false))
            {
                ++count;
            }
        }

        return count;
    }

    Console::Console()
        : m_head(nullptr)
    {
    }


    Console::~Console()
    {
        // on console destruction relink the console functors back to the deferred head
        MoveFunctorsToDeferredHead(V::ConsoleFunctorBase::GetDeferredHead());
    }

    bool Console::PerformCommand
    (
        const char* command,
        ConsoleSilentMode silentMode,
        ConsoleInvokedFrom invokedFrom,
        ConsoleFunctorFlags requiredSet,
        ConsoleFunctorFlags requiredClear
    )
    {
        VStd::string_view commandView;
        ConsoleCommandContainer commandArgsView;
        bool firstIteration = true;
        auto ConvertCommandStringToArray = [&firstIteration, &commandView, &commandArgsView](VStd::string_view token)
        {
            if (firstIteration)
            {
                commandView = token;
                firstIteration = false;
            }
            else
            {
                commandArgsView.emplace_back(token);
            };
        };
        constexpr VStd::string_view commandSeparators = " \t\n\r";
        StringFunc::TokenizeVisitor(command, ConvertCommandStringToArray, commandSeparators);

        return PerformCommand(commandView, commandArgsView, silentMode, invokedFrom, requiredSet, requiredClear);
    }

    bool Console::PerformCommand
    (
        const ConsoleCommandContainer& commandAndArgs,
        ConsoleSilentMode silentMode,
        ConsoleInvokedFrom invokedFrom,
        ConsoleFunctorFlags requiredSet,
        ConsoleFunctorFlags requiredClear
    )
    {
        if (commandAndArgs.empty())
        {
            return false;
        }

        return PerformCommand(commandAndArgs.front(), ConsoleCommandContainer(commandAndArgs.begin() + 1, commandAndArgs.end()), silentMode, invokedFrom, requiredSet, requiredClear);
    }

    bool Console::PerformCommand
    (
        VStd::string_view command,
        const ConsoleCommandContainer& commandArgs,
        ConsoleSilentMode silentMode,
        ConsoleInvokedFrom invokedFrom,
        ConsoleFunctorFlags requiredSet,
        ConsoleFunctorFlags requiredClear
    )
    {
        return DispatchCommand(command, commandArgs, silentMode, invokedFrom, requiredSet, requiredClear);
    }

    void Console::ExecuteConfigFile(VStd::string_view configFileName)
    {
       
        V::IO::PathView configFile(configFileName);
        if (configFile.Extension() == ".setreg")
        {
           
        }
        else if (configFile.Extension() == ".setregpatch")
        {
            
        }
        else
        {
       
        }
    }

    void Console::ExecuteCommandLine(const V::CommandLine& commandLine)
    {
        for (auto&& commandArgument : commandLine)
        {
            const auto& switchKey = commandArgument.m_option;
            const auto& switchValue = commandArgument.m_value;
            ConsoleCommandContainer commandArgs{ switchValue };
            PerformCommand(switchKey, commandArgs, ConsoleSilentMode::NotSilent, ConsoleInvokedFrom::VelcroConsole, ConsoleFunctorFlags::Null, ConsoleFunctorFlags::Null);
        }
    }

    bool Console::ExecuteDeferredConsoleCommands()
    {
        auto DeferredCommandCallable = [this](const DeferredCommand& deferredCommand)
        {
            return this->DispatchCommand(deferredCommand.m_command, deferredCommand.m_arguments, deferredCommand.m_silentMode,
                deferredCommand.m_invokedFrom, deferredCommand.m_requiredSet, deferredCommand.m_requiredClear);
        };
        // Attempt to invoke the deferred command and remove it from the queue if successful
        return VStd::erase_if(m_deferredCommands, DeferredCommandCallable) != 0;
    }

    void Console::ClearDeferredConsoleCommands()
    {
        m_deferredCommands = {};
    }

    bool Console::IsCommand(VStd::string_view command)
    {
        return FindCommand(command) != nullptr;
    }

    ConsoleFunctorBase* Console::FindCommand(VStd::string_view command)
    {
        CVarFixedString lowerName(command);
        VStd::to_lower(lowerName.begin(), lowerName.end());

        CommandMap::iterator iter = m_commands.find(lowerName);
        if (iter != m_commands.end())
        {
            for (ConsoleFunctorBase* curr : iter->second)
            {
                if ((curr->GetFlags() & ConsoleFunctorFlags::IsInvisible) == ConsoleFunctorFlags::IsInvisible)
                {
                    // Filter functors marked as invisible
                    continue;
                }
                return curr;
            }
        }
        return nullptr;
    }

    VStd::string Console::AutoCompleteCommand(VStd::string_view command, VStd::vector<VStd::string>* matches)
    {
        if (command.empty())
        {
            return command;
        }

        ConsoleCommandContainer commandSubset;

        for (ConsoleFunctorBase* curr = m_head; curr != nullptr; curr = curr->m_next)
        {
            if ((curr->GetFlags() & ConsoleFunctorFlags::IsInvisible) == ConsoleFunctorFlags::IsInvisible)
            {
                // Filter functors marked as invisible
                continue;
            }

            if (StringFunc::StartsWith(curr->m_name, command, false))
            {
                VELCROLOG_INFO("- %s : %s\n", curr->m_name, curr->m_desc);
                commandSubset.push_back(curr->m_name);
                if (matches)
                {
                    matches->push_back(curr->m_name);
                }
            }
        }

        VStd::string largestSubstring = command;
        
        if ((!largestSubstring.empty()) && (!commandSubset.empty()))
        {
            const uint32_t totalCount = CountMatchingPrefixes(command, commandSubset);

            for (size_t i = largestSubstring.length(); i < commandSubset.front().length(); ++i)
            {
                const VStd::string nextSubstring = largestSubstring + commandSubset.front()[i];
                const uint32_t count = CountMatchingPrefixes(nextSubstring, commandSubset);

                if (count < totalCount)
                {
                    break;
                }

                largestSubstring = nextSubstring;
            }
        }

        return largestSubstring;
    }

    void Console::VisitRegisteredFunctors(const FunctorVisitor& visitor)
    {
        for (auto& curr : m_commands)
        {
            visitor(curr.second.front());
        }
    }

    void Console::RegisterFunctor(ConsoleFunctorBase* functor)
    {
        if (functor->m_console && (functor->m_console != this))
        {
            V_Assert(false, "Functor is already registered to a console");
            return;
        }

        CVarFixedString lowerName = functor->GetName();
        VStd::to_lower(lowerName.begin(), lowerName.end());
        CommandMap::iterator iter = m_commands.find(lowerName);
        if (iter != m_commands.end())
        {
            // Validate we haven't already added this cvar
            VStd::vector<ConsoleFunctorBase*>::iterator iter2 = VStd::find(iter->second.begin(), iter->second.end(), functor);
            if (iter2 != iter->second.end())
            {
                V_Assert(false, "Duplicate functor registered to the console");
                return;
            }

            // If multiple cvars are registered with the same name, validate that the types and flags match
            if (!iter->second.empty())
            {
                ConsoleFunctorBase* front = iter->second.front();
                if (front->GetFlags() != functor->GetFlags() || front->GetTypeId() != functor->GetTypeId())
                {
                    V_Assert(false, "Mismatched console functor types registered under the same name");
                    return;
                }

                // Discard duplicate functors if the 'DontDuplicate' flag has been set
                if ((front->GetFlags() & ConsoleFunctorFlags::DontDuplicate) != ConsoleFunctorFlags::Null)
                {
                    return;
                }
            }
        }
        m_commands[lowerName].emplace_back(functor);
        functor->Link(m_head);
        functor->m_console = this;
    }

    void Console::UnregisterFunctor(ConsoleFunctorBase* functor)
    {
        if (functor->m_console != this)
        {
            V_Assert(false, "Unregistering a functor bound to a different console");
            return;
        }

        CVarFixedString lowerName = functor->GetName();
        VStd::to_lower(lowerName.begin(), lowerName.end());
        CommandMap::iterator iter = m_commands.find(lowerName);
        if (iter != m_commands.end())
        {
            VStd::vector<ConsoleFunctorBase*>::iterator iter2 = VStd::find(iter->second.begin(), iter->second.end(), functor);
            if (iter2 != iter->second.end())
            {
                iter->second.erase(iter2);
            }
        }
        functor->Unlink(m_head);
        functor->m_console = nullptr;
    }

    void Console::LinkDeferredFunctors(ConsoleFunctorBase*& deferredHead)
    {
        ConsoleFunctorBase* curr = deferredHead;

        while (curr != nullptr)
        {
            ConsoleFunctorBase* next = curr->m_next;
            curr->Unlink(deferredHead);
            RegisterFunctor(curr);
            curr->m_isDeferred = false;
            curr = next;
        }

        deferredHead = nullptr;
    }

    void Console::MoveFunctorsToDeferredHead(ConsoleFunctorBase*& deferredHead)
    {
        m_commands.clear();

        // Re-initialize all of the current functors to a deferred state
        for (ConsoleFunctorBase* curr = m_head; curr != nullptr; curr = curr->m_next)
        {
            curr->m_console = nullptr;
            curr->m_isDeferred = true;
        }

        // If the deferred head contains unregistered functors
        // move this V::Console functors list to the end of the deferred functors
        if (deferredHead)
        {
            ConsoleFunctorBase* oldDeferred = deferredHead;
            while (oldDeferred->m_next != nullptr)
            {
                oldDeferred = oldDeferred->m_next;
            }
            oldDeferred->Link(m_head);
        }
        else
        {
            deferredHead = m_head;
        }

        ConsoleFunctorBase::_deferredHeadInvoked = false;

        m_head = nullptr;
    }

    bool Console::DispatchCommand
    (
        VStd::string_view command,
        const ConsoleCommandContainer& inputs,
        ConsoleSilentMode silentMode,
        ConsoleInvokedFrom invokedFrom,
        ConsoleFunctorFlags requiredSet,
        ConsoleFunctorFlags requiredClear
    )
    {
        bool result = false;
        ConsoleFunctorFlags flags = ConsoleFunctorFlags::Null;

        CVarFixedString lowerName(command);
        VStd::to_lower(lowerName.begin(), lowerName.end());

        CommandMap::iterator iter = m_commands.find(lowerName);
        if (iter != m_commands.end())
        {
            for (ConsoleFunctorBase* curr : iter->second)
            {
                if ((curr->GetFlags() & requiredSet) != requiredSet)
                {
                    VELCROLOG_WARN("%s failed required set flag check\n", curr->m_name);
                    continue;
                }

                if ((curr->GetFlags() & requiredClear) != ConsoleFunctorFlags::Null)
                {
                    VELCROLOG_WARN("%s failed required clear flag check\n", curr->m_name);
                    continue;
                }

                if ((curr->GetFlags() & ConsoleFunctorFlags::IsCheat) != ConsoleFunctorFlags::Null)
                {
                    VELCROLOG_WARN("%s is marked as a cheat\n", curr->m_name);
                }

                if ((curr->GetFlags() & ConsoleFunctorFlags::IsDeprecated) != ConsoleFunctorFlags::Null)
                {
                    VELCROLOG_WARN("%s is marked as deprecated\n", curr->m_name);
                }

                if ((curr->GetFlags() & ConsoleFunctorFlags::NeedsReload) != ConsoleFunctorFlags::Null)
                {
                    VELCROLOG_WARN("Changes to %s will only take effect after level reload\n", curr->m_name);
                }

                // Letting this intentionally fall-through, since in editor we can register common variables multiple times
                (*curr)(inputs);

                if (!result)
                {
                    result = true;
                    if ((silentMode == ConsoleSilentMode::NotSilent) && (curr->GetFlags() & ConsoleFunctorFlags::IsInvisible) != ConsoleFunctorFlags::IsInvisible)
                    {
                        CVarFixedString value;
                        curr->GetValue(value);
                        VELCROLOG_INFO("> %s : %s\n", curr->GetName(), value.empty() ? "<empty>" : value.c_str());
                    }
                    flags = curr->GetFlags();
                }
            }
        }

        if (result)
        {
            m_consoleCommandInvokedEvent.Signal(command, inputs, flags, invokedFrom);
        }
        else
        {
            m_dispatchCommandNotFoundEvent.Signal(command, inputs, invokedFrom);
        }

        return result;
    }
}


