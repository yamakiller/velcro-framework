#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_INTERNAL_TYPE_SEQUENCE_TRAITS_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_INTERNAL_TYPE_SEQUENCE_TRAITS_H

#include <core/std/typetraits/integral_constant.h>

namespace VStd {
    namespace Internal {
        template <typename ...Ts>
        struct pack_traits_arg_sequence {};

        template <size_t index, typename T>
        struct pack_traits_get_arg;

        template <size_t index>
        struct pack_traits_get_arg<index, pack_traits_arg_sequence<>>
        {
            static_assert(VStd::integral_constant<size_t, index>::value && false, "Element index out of bounds");
        };

        template <typename First, typename ...Rest>
        struct pack_traits_get_arg<0, pack_traits_arg_sequence<First, Rest...>>
        {
            using type = First;
        };

        template <size_t index, typename First, typename ...Rest>
        struct pack_traits_get_arg<index, pack_traits_arg_sequence<First, Rest...>> : pack_traits_get_arg<index - 1, pack_traits_arg_sequence<Rest...>> {};

        template <size_t index, typename ...Args>
        using pack_traits_get_arg_t = typename pack_traits_get_arg<index, pack_traits_arg_sequence<Args...>>::type;
    }
}

#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_INTERNAL_TYPE_SEQUENCE_TRAITS_H