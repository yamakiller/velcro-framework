#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_CONDITIONAL_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_CONDITIONAL_H

#include <core/std/typetraits/config.h>

namespace VStd {
    using std::conditional;
    using std::conditional_t;

    using std::enable_if;
    using std::enable_if_t;
} // namespace VStd


// Backwards comaptible VStd extensions for std::conditional and std::enable_if
namespace VStd::Utils {
    // Use VStd::conditional_t instead
    template<bool Condition, typename T, typename F>
    using if_c = VStd::conditional<Condition, T, F>;
    // Use VStd::enable_if_t instead
    template<bool Condition, typename T = void>
    using enable_if_c = VStd::enable_if<Condition, T>;
}

#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_CONDITIONAL_H