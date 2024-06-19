#ifndef V_FRAMEWORK_CORE_STD_FUNCTION_INVOKE_H
#define V_FRAMEWORK_CORE_STD_FUNCTION_INVOKE_H

#include <vcore/std/typetraits/invoke_traits.h>

namespace VStd {
    template <class Fn, class... ArgTypes>
    struct is_invocable : Internal::invocable<Fn, ArgTypes...>::type {};

    template <class R, class Fn, class... ArgTypes>
    struct is_invocable_r : Internal::invocable_r<R, Fn, ArgTypes...>::type {};

    template <class Fn, class... ArgTypes>
    constexpr bool is_invocable_v = is_invocable<Fn, ArgTypes...>::value;

    template <class R, class Fn, class ...ArgTypes>
    constexpr bool is_invocable_r_v = is_invocable_r<R, Fn, ArgTypes...>::value;

    template<class Fn, class... ArgTypes>
    struct invoke_result
        : VStd::enable_if<Internal::invocable<Fn, ArgTypes...>::value, typename Internal::invocable<Fn, ArgTypes...>::result_type>
    {};

    template<class Fn, class... ArgTypes>
    using invoke_result_t = typename invoke_result<Fn, ArgTypes...>::type;

    template <class F, class... Args>
    inline constexpr invoke_result_t<F, Args...> invoke(F&& f, Args&&... args) {
        return Internal::INVOKE(Internal::InvokeTraits::forward<F>(f), Internal::InvokeTraits::forward<Args>(args)...);
    }
}

#endif // namespace V_FRAMEWORK_CORE_STD_FUNCTION_INVOKE_H