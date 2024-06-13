/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_INVOKE_TRAITS_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_INVOKE_TRAITS_H

#include <core/std/reference_wrapper.h>
#include <core/std/typetraits/add_reference.h>
#include <core/std/typetraits/conditional.h>
#include <core/std/typetraits/is_base_of.h>
#include <core/std/typetraits/is_convertible.h>
#include <core/std/typetraits/is_member_function_pointer.h>
#include <core/std/typetraits/is_member_object_pointer.h>
#include <core/std/typetraits/is_void.h>
#include <core/std/typetraits/remove_reference.h>

namespace VStd {
    namespace Internal {
        // Represents non-usable type which is not equal to any other type
        struct nat final {
            nat() = delete;
            ~nat() = delete;
            nat(const nat&) = delete;
            nat& operator=(const nat&) = delete;
        };

        // Represents a type that can accept any argument
        struct param_any final {
            param_any(...);
        };

        template <class... T> struct check_complete_type;

        template <>
        struct check_complete_type<> {
        };

        template <class T0, class T1, class... Ts>
        struct check_complete_type<T0, T1, Ts...>
            : private check_complete_type<T0>
            , private check_complete_type<T1, Ts...> {
        };

        template <class T>
        struct check_complete_type<T> {
            static_assert((sizeof(T) > 0), "Type must be complete.");
        };

        template <class T>
        struct check_complete_type<T&> : private check_complete_type<T>{};

        template <class T>
        struct check_complete_type<T&&> : private check_complete_type<T> {};

        template <class R, class... Args>
        struct check_complete_type<R(*)(Args...)> : private check_complete_type<R> {};

        template <class R, class... Args>
        struct check_complete_type<R(*)(Args...) noexcept> : private check_complete_type<R> {};

        template <class... Args>
        struct check_complete_type<void(*)(Args...)> {};

        template <class... Args>
        struct check_complete_type<void(*)(Args...) noexcept> {};

        template <class R, class... Args>
        struct check_complete_type<R(Args...)> : private check_complete_type<R> {};

        template <class R, class... Args>
        struct check_complete_type<R(Args...) noexcept> : private check_complete_type<R> {};

        template <class... Args>
        struct check_complete_type<void(Args...)> {};

        template <class... Args>
        struct check_complete_type<void(Args...) noexcept> {};

        template <class R, class... Args>
        struct check_complete_type<R(*)(Args..., ...)> : private check_complete_type<R> {};

        template <class R, class... Args>
        struct check_complete_type<R(*)(Args..., ...) noexcept> : private check_complete_type<R> {};

        template <class... Args>
        struct check_complete_type<void(*)(Args..., ...)> {};

        template <class... Args>
        struct check_complete_type<void(*)(Args..., ...) noexcept> {};

        template <class R, class... Args>
        struct check_complete_type<R(Args..., ...)> : private check_complete_type<R> {};

        template <class R, class... Args>
        struct check_complete_type<R(Args..., ...) noexcept> : private check_complete_type<R> {};

        template <class... Args>
        struct check_complete_type<void(Args..., ...)> {};

        template <class... Args>
        struct check_complete_type<void(Args..., ...) noexcept> {};

        template <class R, class ClassType>
        struct check_complete_type<R ClassType::*> : private check_complete_type<ClassType> {};

        template<class T>
        struct member_pointer_class_type {};

        template<class R, class ClassType>
        struct member_pointer_class_type<R ClassType::*>
        {
            using type = ClassType;
        };

        template<class T>
        using member_pointer_class_type_t = typename member_pointer_class_type<T>::type;

        namespace InvokeTraits
        {
            template <class T>
            add_rvalue_reference_t<T> declval();

            template<class T>
            constexpr inline T && forward(VStd::remove_reference_t<T>& u)
            {
                return static_cast<T &&>(u);
            }

            template<class T>
            constexpr inline T && forward(VStd::remove_reference_t<T>&& u)
            {
                return static_cast<T &&>(u);
            }
        }

        // 23.14.3 [func.require]
        // fallback INVOKE declaration
        // Allows it to be used in unevaluated context such as is_invocable when none of the 7 bullet points apply
        template<class... Args>
        auto INVOKE(param_any, Args&&...) -> nat;

        // Below are the 7 implementations of INVOKE bullet point
        // 1.1
        template<class Fn, class Arg0, class... Args, typename = VStd::enable_if_t<
            VStd::is_member_function_pointer<VStd::decay_t<Fn>>::value
            && VStd::is_base_of<member_pointer_class_type_t<VStd::decay_t<Fn>>, VStd::decay_t<Arg0>>::value>>
            constexpr auto INVOKE(Fn&& f, Arg0&& arg0, Args&&... args) -> decltype((InvokeTraits::forward<Arg0>(arg0).*f)(InvokeTraits::forward<Args>(args)...))
        {
            return (InvokeTraits::forward<Arg0>(arg0).*f)(InvokeTraits::forward<Args>(args)...);
        }
        // 1.2
        template<class Fn, class Arg0, class... Args, typename = VStd::enable_if_t<
                VStd::is_member_function_pointer<VStd::decay_t<Fn>>::value
                && VStd::is_reference_wrapper<VStd::decay_t<Arg0>>::value>>
            constexpr auto INVOKE(Fn&& f, Arg0&& arg0, Args&&... args) -> decltype((arg0.get().*f)(InvokeTraits::forward<Args>(args)...))
        {
            return (arg0.get().*f)(InvokeTraits::forward<Args>(args)...);
        }
        // 1.3
        template<class Fn, class Arg0, class... Args, typename = VStd::enable_if_t<
                VStd::is_member_function_pointer<VStd::decay_t<Fn>>::value
                && !VStd::is_base_of<member_pointer_class_type_t<VStd::decay_t<Fn>>, VStd::decay_t<Arg0>>::value
                && !VStd::is_reference_wrapper<VStd::decay_t<Arg0>>::value>>
            constexpr auto INVOKE(Fn&& f, Arg0&& arg0, Args&&... args) -> decltype(((*InvokeTraits::forward<Arg0>(arg0)).*f)(InvokeTraits::forward<Args>(args)...))
        {
            return ((*InvokeTraits::forward<Arg0>(arg0)).*f)(InvokeTraits::forward<Args>(args)...);
        }
        // 1.4
        template<class Fn, class Arg0, typename = VStd::enable_if_t<
                VStd::is_member_object_pointer<VStd::decay_t<Fn>>::value
                && VStd::is_base_of<member_pointer_class_type_t<VStd::decay_t<Fn>>, VStd::decay_t<Arg0>>::value>>
            constexpr auto INVOKE(Fn&& f, Arg0&& arg0) -> decltype(InvokeTraits::forward<Arg0>(arg0).*f)
        {
            return InvokeTraits::forward<Arg0>(arg0).*f;
        }
        // 1.5
        template<class Fn, class Arg0, typename = VStd::enable_if_t<
                VStd::is_member_object_pointer<VStd::decay_t<Fn>>::value
                && VStd::is_reference_wrapper<VStd::decay_t<Arg0>>::value>>
            constexpr auto INVOKE(Fn&& f, Arg0&& arg0) -> decltype(arg0.get().*f)
        {
            return arg0.get().*f;
        }
        // 1.6
        template<class Fn, class Arg0, typename = VStd::enable_if_t<
                VStd::is_member_object_pointer<VStd::decay_t<Fn>>::value
                && !VStd::is_base_of<member_pointer_class_type_t<VStd::decay_t<Fn>>, VStd::decay_t<Arg0>>::value
                && !VStd::is_reference_wrapper<VStd::decay_t<Arg0>>::value>>
            constexpr auto INVOKE(Fn&& f, Arg0&& arg0) -> decltype((*InvokeTraits::forward<Arg0>(arg0)).*f)
        {
            return (*InvokeTraits::forward<Arg0>(arg0)).*f;
        }
        // 1.7
        template<class Fn, class... Args>
        constexpr auto INVOKE(Fn&& f, Args&&... args) -> decltype(InvokeTraits::forward<Fn>(f)(InvokeTraits::forward<Args>(args)...))
        {
            return InvokeTraits::forward<Fn>(f)(InvokeTraits::forward<Args>(args)...);
        }

        template<class R, class Fn, class... ArgTypes>
        struct invocable_r : private check_complete_type<Fn>
        {
            template<class Func, class ... Args>
            static auto try_call(int) -> decltype(INVOKE(InvokeTraits::declval<Func>(), InvokeTraits::declval<Args>()...));
            template<class Func, class... Args>
            static nat try_call(...);

            using result_type = decltype(try_call<Fn, ArgTypes...>(0));
            using type = VStd::conditional_t<!VStd::is_same<result_type, nat>::value,
                VStd::conditional_t<!VStd::is_void<R>::value, VStd::is_convertible<result_type, R>, VStd::true_type>,
                VStd::false_type>;
            static constexpr bool value = type::value;
        };

        template<class Fn, class... ArgTypes>
        using invocable = invocable_r<void, Fn, ArgTypes...>;
    }
}

#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_INVOKE_TRAITS_H