#include <vcore/vcore_traits_platform.h>
#include <vcore/std/functional.h>
#include <vcore/std/string/conversions.h>
#include <vcore/settings/command_line.h>
#include <vcore/string_func/string_func.h>

namespace V
{
    namespace
    {
        // helper utility to return a lower version of the string without altering the original.
        // regular to_lower operates directly on the input.
        VStd::string ToLower(VStd::string_view inStr)
        {
            VStd::string lowerStr = inStr;
            VStd::to_lower(lowerStr.begin(), lowerStr.end());

            return lowerStr;
        }

        VStd::string_view UnquoteArgument(VStd::string_view arg)
        {
            if (arg.size() < 2)
            {
                return arg;
            }

            return arg.front() == '"' && arg.back() == '"' ? VStd::string_view{ arg.begin() + 1, arg.end() - 1 } : arg;
        }

        VStd::string QuoteArgument(VStd::string_view arg)
        {
            return !arg.empty() ? VStd::string::format(R"("%.*s")", v_numeric_cast<int>(arg.size()), arg.data()) : VStd::string{ arg };
        }
    }

    CommandLine::CommandLine() : m_commandLineOptionPrefix(V_TRAIT_COMMAND_LINE_OPTION_PREFIX)
    {
    }

    CommandLine::CommandLine(VStd::string_view commandLineOptionPrefix)
        : m_commandLineOptionPrefix(commandLineOptionPrefix)
    {
    }

    void CommandLine::ParseOptionArgument(VStd::string_view newOption, VStd::string_view newValue,
        CommandArgument* inProgressArgument)
    {
        // Allow argument values wrapped in quotes at both ends to become the value within the quotes
        VStd::string_view unquotedValue = UnquoteArgument(newValue);
        if (unquotedValue != newValue)
        {
            // Update the inProgressArgument before adding new arguments
            if(inProgressArgument)
            {
                inProgressArgument->m_value = unquotedValue;
                // Set the inProgressArgument to nullptr to indicate that the inProgressArgument has been fulfilled
                inProgressArgument = nullptr;
            }
            else
            {
                m_allValues.push_back({ newOption, unquotedValue });
            }
        }
        else
        {
            VStd::vector<VStd::string_view> tokens;
            auto splitArgument = [&tokens](VStd::string_view token)
            {
                tokens.emplace_back(V::StringFunc::StripEnds(token));
            };
            V::StringFunc::TokenizeVisitor(newValue, splitArgument, ",;");

            // we do it this way because you are allowed to do odd things like
            // -root=abc -root=hij,klm -root whee -root fun;days
            // and roots value should be { abc, hij, klm, whee, fun, days }
            for (VStd::string_view optionValue : tokens)
            {
                if (inProgressArgument)
                {
                    inProgressArgument->m_value = optionValue;
                    // Set the inProgressArgument to nullptr to indicate that the inProgressArgument has been fulfilled
                    inProgressArgument = nullptr;
                }
                else
                {
                    m_allValues.push_back({ newOption, optionValue });
                }
            }
        }
    }

    void CommandLine::AddArgument(VStd::string_view currentArg, VStd::string& currentSwitch)
    {
        currentArg = V::StringFunc::StripEnds(currentArg);
        if (!currentArg.empty())
        {
            if (m_commandLineOptionPrefix.contains(currentArg.front()))
            {
                // its possible that its a key-value-pair like -blah=whatever
                // we support this too, for compatibility.

                currentArg = currentArg.substr(1);
                if (currentArg[0] == '-') // for -- extra
                {
                    currentArg = currentArg.substr(1);
                }

                VStd::size_t foundPos = V::StringFunc::Find(currentArg, "=");
                if (foundPos != VStd::string::npos)
                {
                    VStd::string_view argumentView{ currentArg };
                    VStd::string_view option = V::StringFunc::StripEnds(argumentView.substr(0, foundPos));
                    VStd::string_view value = V::StringFunc::StripEnds(argumentView.substr(foundPos + 1));
                    ParseOptionArgument(ToLower(option), value, nullptr);
                    currentSwitch.clear();
                }
                else
                {
                    // its in this format -switchName switchvalue
                    // (no equals)
                    currentSwitch = ToLower(currentArg);
                    m_allValues.push_back({ currentSwitch, "" });
                }
            }
            else
            {
                if (currentSwitch.empty())
                {
                    m_allValues.push_back({ "", UnquoteArgument(currentArg) });
                }
                else
                {
                    ParseOptionArgument(currentSwitch, currentArg, &m_allValues.back());
                    currentSwitch.clear();
                }
            }
        }
    }

    void CommandLine::Parse(int argc, char** argv)
    {
        m_allValues.clear();

        VStd::string currentSwitch;
        // Start on 1 because 0 is the executable name
        for (int i = 1; i < argc; ++i)
        {
            if (argv[i])
            {
                VStd::string currentArg = argv[i]; // this eats the / or -
                AddArgument(currentArg, currentSwitch);
            }
        }
    }

    void CommandLine::Parse(const ParamContainer& commandLine)
    {
        m_allValues.clear();

        // This version of Parse does not skip over 0th index
        VStd::string currentSwitch;
        for (int i = 0; i < commandLine.size(); ++i)
        {
            AddArgument(commandLine[i], currentSwitch);
        }
    }

    void CommandLine::Dump(ParamContainer& commandLineDumpOutput) const
    {
        V_Error("CommandLine", !m_commandLineOptionPrefix.empty(),
            "Cannot dump command line switches from a command line with an empty option prefix");

        for (const CommandArgument& argument : m_allValues)
        {
            if (!argument.m_option.empty())
            {
                commandLineDumpOutput.emplace_back(m_commandLineOptionPrefix.front() + argument.m_option);
            }
            if (!argument.m_value.empty())
            {
                commandLineDumpOutput.emplace_back(QuoteArgument(argument.m_value));
            }
        }
    }

    bool CommandLine::HasSwitch(VStd::string_view switchName) const
    {
        auto commandArgumentIter = VStd::find_if(m_allValues.begin(), m_allValues.end(),
            [optionName = ToLower(switchName)](const CommandArgument& argument) { return argument.m_option == optionName; });
        return commandArgumentIter != m_allValues.end();
    }

    VStd::size_t CommandLine::GetNumSwitchValues(VStd::string_view switchName) const
    {
        return VStd::count_if(m_allValues.begin(), m_allValues.end(),
            [optionName = ToLower(switchName)](const CommandArgument& argument) { return argument.m_option == optionName; });
    }

    const VStd::string& CommandLine::GetSwitchValue(VStd::string_view switchName, VStd::size_t index) const
    {
        VStd::string optionName = ToLower(switchName);
        size_t currentPosIndex{};
        auto findArgumentAt = [&optionName, index, &currentPosIndex](const CommandArgument& argument)
        {
            if (argument.m_option == optionName)
            {
                return currentPosIndex++ == index;
            }

            return false;
        };
        auto commandArgumentIter = VStd::find_if(m_allValues.begin(), m_allValues.end(), findArgumentAt);
        if (!optionName.empty())
        {
            V_Assert(index < currentPosIndex, R"(Invalid Command line optional argument lookup of "%s" at index %zu)",
                optionName.c_str(), index);
        }
        else
        {
            V_Assert(index < currentPosIndex, R"(Invalid Command line positional argument lookup at index %zu)", index);
        }
        if (commandArgumentIter == m_allValues.end())
        {
            return m_emptyValue;
        }
        return commandArgumentIter->m_value;
    }

    VStd::size_t CommandLine::GetNumMiscValues() const
    {
        return GetNumSwitchValues("");
    }

    const VStd::string& CommandLine::GetMiscValue(VStd::size_t index) const
    {
        // Positional arguments option value is an empty string
        return GetSwitchValue("", index);
    }

    [[nodiscard]] bool CommandLine::empty() const
    {
        return m_allValues.empty();
    }

    auto CommandLine::size() const -> ArgumentVector::size_type
    {
        return m_allValues.size();
    }
    auto CommandLine::begin() -> ArgumentVector::iterator
    {
        return m_allValues.begin();
    }
    auto CommandLine::begin() const -> ArgumentVector::const_iterator
    {
        return m_allValues.begin();
    }
    auto CommandLine::cbegin() const -> ArgumentVector::const_iterator
    {
        return m_allValues.cbegin();
    }
    auto CommandLine::end() -> ArgumentVector::iterator
    {
        return m_allValues.end();
    }
    auto CommandLine::end() const -> ArgumentVector::const_iterator
    {
        return m_allValues.end();
    }
    auto CommandLine::cend() const -> ArgumentVector::const_iterator
    {
        return m_allValues.cend();
    }
    auto CommandLine::rbegin() -> ArgumentVector::reverse_iterator
    {
        return m_allValues.rbegin();
    }
    auto CommandLine::rbegin() const -> ArgumentVector::const_reverse_iterator
    {
        return m_allValues.rbegin();
    }
    auto CommandLine::crbegin() const -> ArgumentVector::const_reverse_iterator
    {
        return m_allValues.crbegin();
    }
    auto CommandLine::rend() -> ArgumentVector::reverse_iterator
    {
        return m_allValues.rend();
    }
    auto CommandLine::rend() const -> ArgumentVector::const_reverse_iterator
    {
        return m_allValues.rend();
    }
    auto CommandLine::crend() const -> ArgumentVector::const_reverse_iterator
    {
        return m_allValues.crend();
    }
}
