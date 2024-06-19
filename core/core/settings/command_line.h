#ifndef V_FRAMEWORK_CORE_SETTINGS_COMMAND_LINE_H
#define V_FRAMEWORK_CORE_SETTINGS_COMMAND_LINE_H

#include <core/base.h>
#include <core/memory/system_allocator.h>
#include <core/std/containers/unordered_map.h>
#include <core/std/string/fixed_string.h>
#include <core/std/string/string.h>
#include <core/std/string/string_view.h>

namespace V {
    class CommandLine {
    public:
        V_CLASS_ALLOCATOR(CommandLine, V::SystemAllocator, 0);

        using ParamContainer = VStd::vector<VStd::string>;

        struct CommandArgument
        {
            VStd::string m_option;
            VStd::string m_value;
        };
        using ArgumentVector = VStd::vector<CommandArgument>;

        CommandLine();

        /**
         * Initializes a CommandLine instance which uses the provided commandLineOptionPreix for parsing switches
         */
        CommandLine(VStd::string_view commandLineOptionPrefix);
        /**
        * Construct a command line parser.
        * It will load parameters from the given ARGC/ARGV parameters instead of process command line.
        * Skips over the first parameter as it assumes it is the executable name
        */
        void Parse(int argc, char** argv);
        /**
         * Parses each element of the command line as parameter.
         * Unlike the ARGC/ARGV version above, this function doesn't skip over the first parameter
         * It allows for round trip conversion with the Dump() method
         */
        void Parse(const ParamContainer& commandLine);

        /**
        * Will dump command line parameters from the CommandLine in a format such that switches
        * are prefixed with the option prefix followed by their value(s) which are comma separated
        * The result of this function can be supplied to Parse() to re-create an equivalent command line object
        * Ex, If the command line has the current list of parsed miscellaneous values and switches of
        * MiscValue = ["Foo", "Bat"]
        * Switches = ["GameFolder" : [], "RemoteIp" : ["10.0.0.1"], "ScanFolders" : ["\a\b\c", "\d\e\f"]
        * CommandLineOptionPrefix = "-/"
        *
        * Then the resulting dumped value would be
        * Dump = ["Foo", "Bat", "-GameFolder", "-RemoteIp", "10.0.0.1", "-ScanFolders", "\a\b\c", "-ScanFolders", "\d\e\f"]
        */
        void Dump(ParamContainer& commandLineDumpOutput) const;

        /**
        * Determines whether a switch is present in the command line
        */
        bool HasSwitch(VStd::string_view switchName) const;

        /**
        * Get the number of values for the given switch.
        * @return 0 if the switch does not exist, otherwise the total number of values that appear after that switch
        */
        VStd::size_t GetNumSwitchValues(VStd::string_view switchName) const;

        /**
        * Get the actual value of a switch
        * @param switchName The switch to search for
        * @param index The 0-based index to retrieve the switch value for
        * @return The value at that index.  This will Assert if you attempt to index out of bounds
        */
        const VStd::string& GetSwitchValue(VStd::string_view switchName, VStd::size_t index) const;

        /*
        * Get the number of misc values (values that are not associated with a switch)
        */
        VStd::size_t GetNumMiscValues() const;

        /*
        * Given an index, return the actual value of the misc value at that index
        * This will assert if you attempt to index out of bounds.
        */
        const VStd::string& GetMiscValue(VStd::size_t index) const;


        // Range accessors
        [[nodiscard]] bool empty() const;
        auto size() const -> ArgumentVector::size_type;
        auto begin() -> ArgumentVector::iterator;
        auto begin() const -> ArgumentVector::const_iterator;
        auto cbegin() const -> ArgumentVector::const_iterator;
        auto end() -> ArgumentVector::iterator;
        auto end() const -> ArgumentVector::const_iterator;
        auto cend() const -> ArgumentVector::const_iterator;
        auto rbegin() -> ArgumentVector::reverse_iterator;
        auto rbegin() const -> ArgumentVector::const_reverse_iterator;
        auto crbegin() const -> ArgumentVector::const_reverse_iterator;
        auto rend() -> ArgumentVector::reverse_iterator;
        auto rend() const -> ArgumentVector::const_reverse_iterator;
        auto crend() const -> ArgumentVector::const_reverse_iterator;

    private:
        void AddArgument(VStd::string_view currentArg, VStd::string& currentSwitch);
        void ParseOptionArgument(VStd::string_view newOption, VStd::string_view newValue, CommandArgument* inProgressArgument);

        ArgumentVector m_allValues;
        VStd::string m_emptyValue;

        inline static constexpr size_t MaxCommandOptionPrefixes = 8;
        VStd::fixed_string<MaxCommandOptionPrefixes> m_commandLineOptionPrefix;
    };
}


#endif // V_FRAMEWORK_CORE_SETTINGS_COMMAND_LINE_H