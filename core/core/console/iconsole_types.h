#ifndef V_FRAMEWORK_CORE_CONSOLE_ICONSOLE_TYPE_H
#define V_FRAMEWORK_CORE_CONSOLE_ICONSOLE_TYPE_H

#include <core/base.h>
#include <core/std/typetraits/underlying_type.h>

// VStd forwards
namespace VStd
{
    template <class Element>
    struct char_traits;

    template <class Element, class Traits>
    class basic_string_view;

    template <class Element, size_t MaxElementCount, class Traits>
    class basic_fixed_string;
    
    template <class T, size_t Capacity>
    class fixed_vector;
}

namespace V
{
    template <class... Params>
    class Event;

    // Provides compile time constants and type aliases for accessing Velcro Console
    // Without the requirement to have a complete console type
    inline constexpr size_t MaxConsoleCommandPlusArgsLength = 64;
    using ConsoleCommandContainer = VStd::fixed_vector<VStd::basic_string_view<char, VStd::char_traits<char>>, MaxConsoleCommandPlusArgsLength>;
    
    inline constexpr size_t MaxCVarStringLength = 256;
    using CVarFixedString = VStd::basic_fixed_string<char, MaxCVarStringLength, VStd::char_traits<char>>;

    enum class ConsoleFunctorFlags
    {
        Null           = 0        // Empty flags
    ,   DontReplicate  = (1 << 0) // Should not be replicated
    ,   ServerOnly     = (1 << 1) // Should never replicate to clients
    ,   ReadOnly       = (1 << 2) // Should not be invoked at runtime
    ,   IsCheat        = (1 << 3) // Command is a cheat, may require escalated privileges to modify
    ,   IsInvisible    = (1 << 4) // Should not be shown in the console for autocomplete
    ,   IsDeprecated   = (1 << 5) // Command is deprecated, show a warning when invoked
    ,   NeedsReload    = (1 << 6) // Level should be reloaded after executing this command
    ,   AllowClientSet = (1 << 7) // Allow clients to modify this cvar even in release (this alters the cvar for all connected servers and clients, be VERY careful enabling this flag)
    ,   DontDuplicate  = (1 << 8) // Discard functors with the same name as another that has already been registered instead of duplicating them (which is the default behavior)
    };
    V_DEFINE_ENUM_BITWISE_OPERATORS(ConsoleFunctorFlags);

    enum class ConsoleSilentMode
    {
        Silent,
        NotSilent
    };

    enum class ConsoleInvokedFrom
    {
        VelcroConsole,
        VelcroNetworking,
        VelcroBinding
    };

    //! Called on invocation of any console command.
    //! @param command     the full command being executed on the console
    //! @param commandArgs the array of arguments that was supplied to PeformCommand
    //! @param flags       the set of flags associated with the command being executed
    //! @param invokedFrom the source point that initiated console invocation
    using ConsoleCommandInvokedEvent = V::Event<VStd::basic_string_view<char, VStd::char_traits<char>>, const ConsoleCommandContainer&, ConsoleFunctorFlags, ConsoleInvokedFrom>;

    //! Called when a command to dispatch has not been found within the Velcro Console.
    //! @param command the full command that was not found
    //! @param commandArgs the array of arguments that was supplied to PeformCommand
    //! @param invokedFrom the source point that initiated console invocation
    using DispatchCommandNotFoundEvent = V::Event<VStd::basic_string_view<char, VStd::char_traits<char>>, const ConsoleCommandContainer&, ConsoleInvokedFrom>;
}


#endif // V_FRAMEWORK_CORE_CONSOLE_ICONSOLE_TYPE_H