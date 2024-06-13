/*
 * Copyright (c) Contributors to the VelcroFramework Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_HAS_MEMBER_FUNCTION_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_HAS_MEMBER_FUNCTION_H


#include <core/std/function/invoke.h>
#include <core/std/typetraits/conditional.h>
#include <core/std/typetraits/remove_cvref.h>
#include <core/variadic.h>

/**
* Helper to create checkers for member function inside a class.
* Explanation of how checks are done:
* 1. Direct Member check:
* This validates that the class itself has the member and that is not being detected from any base class
* The check is done by using the box_member template to enforce that the member function pointer
* is actually from the supplied class type and not from a base type
*
* 2. Member Invocable with Parameters and returns Result check:
* This validates that the member exist on the class and if it is a non-static member
* it can be invoked with a signature of (<class name instance>, Params...)
* or if it is a static member it can be invoked with a signature of (Params...)
* and returns the result type R
*/
// Used to append options to the VStd::is_invocable_r_v below
#define V_PREFIX_ARG_WITH_COMMA(_Arg) , _Arg


namespace VStd::HasMemberInternal
{
    // Function declaration only used in unevaluated context
    // in order to enforce that the member function pointer
    // is a direct member of the class
    template<class T, class MemType, MemType Member, class... Args>
    auto direct_member_box() -> VStd::invoke_result_t<MemType, T, Args...>;
}

/**
* V_HAS_MEMBER detects if the member exists directly on the class and
* can be invoked with the specified function name and signature
*
* NOTE: This does not include base classes in the check
* Ex. V_HAS_MEMBER(IsReadyMember,IsReady,void,())
*     struct A {};
*     struct B
*     {
*        void IsReady();
*     };
*     struct Der : B {};
*     HasIsReadyMember<A>::value (== false)
*     HasIsReadyMember<B>::value (== true)
*     HasIsReadyMember<Der>::value (== false)
*/
#define V_HAS_MEMBER_QUAL(_HasName, _FunctionName, _ResultType, _ParamsSignature, _Qual)          \
    template<class T, class = void>                                                                \
    class Has##_HasName                                                                            \
        : public VStd::false_type                                                                 \
    {                                                                                              \
    };                                                                                             \
    template<class T>                                                                              \
    class Has##_HasName<T, VStd::enable_if_t<VStd::is_invocable_r_v<                             \
            _ResultType,                                                                           \
            decltype(::VStd::HasMemberInternal::direct_member_box<VStd::remove_cvref_t<T>,       \
                _ResultType (VStd::remove_cvref_t<T>::*) _ParamsSignature _Qual,                  \
                &VStd::remove_cvref_t<T>::_FunctionName                                           \
                V_FOR_EACH_UNWRAP(V_PREFIX_ARG_WITH_COMMA, _ParamsSignature)>)>                  \
    >>                                                                                             \
         : public VStd::true_type                                                                 \
    {                                                                                              \
    };                                                                                             \
    template <class T>                                                                             \
    inline static constexpr bool Has##_HasName ## _v = Has##_HasName<T>::value;

#define V_HAS_MEMBER(_HasName, _FunctionName, _ResultType, _ParamsSignature) \
    V_HAS_MEMBER_QUAL(_HasName, _FunctionName, _ResultType, _ParamsSignature,)


/**
* V_HAS_MEMBER_INCLUDE_BASE detects if the class or any of its base classes
* can invoke the member function with the specified function name and signature
* V_HAS_MEMBER_INCLUDE_BASE(IsReadyIncludeBase,IsReady,void,())
*     struct A {};
*     struct B
*     {
*        static void IsReady();
*     };
*     struct Der : B {};
*     HasIsReadyIncludeBase<A>::value (== false)
*     HasIsReadyIncludeBase<B>::value (== true)
*     HasIsReadyIncludeBase<Der>::value (== true)
*/
#define V_HAS_MEMBER_INCLUDE_BASE_QUAL(_HasName, _FunctionName, _ResultType, _ParamsSignature, _Qual) \
    template<class T, class = void>                                                              \
    class Has##_HasName                                                                          \
        : public VStd::false_type                                                               \
    {                                                                                            \
    };                                                                                           \
    template<class T>                                                                            \
    class Has##_HasName<T, VStd::enable_if_t<VStd::is_invocable_r_v<                           \
        _ResultType,                                                                             \
        decltype(static_cast<_ResultType (VStd::remove_cvref_t<T>::*) _ParamsSignature _Qual>(&VStd::remove_cvref_t<T>::_FunctionName)), \
        VStd::remove_cvref_t<T>                                                                 \
        V_FOR_EACH_UNWRAP(V_PREFIX_ARG_WITH_COMMA, _ParamsSignature)>                          \
    >>                                                                                           \
         : public VStd::true_type                                                               \
    {                                                                                            \
    };                                                                                           \
    template <class T>                                                                           \
    inline static constexpr bool Has##_HasName ## _v = Has##_HasName<T>::value;

#define V_HAS_MEMBER_INCLUDE_BASE(_HasName, _FunctionName, _ResultType, _ParamsSignature) \
    V_HAS_MEMBER_INCLUDE_BASE_QUAL(_HasName, _FunctionName, _ResultType, _ParamsSignature,)

/**
* V_HAS_STATIC_MEMBER detects if the class or any of its base classes
* can invoke the static member function with the specified function name and signature
* NOTE: There is no way to validate if the static member function was directly
* declared on the supplied class. This is due to static member functions NOT containing
* the class type as part of the function signature
* Ex. V_HAS_STATIC_MEMBER(IsReadyStaticMember,IsReady,void,())
*     struct A {};
*     struct B
*     {
*        static void IsReady();
*     };
*     struct Der : B {};
*     HasIsReadyStaticMember<A>::value (== false)
*     HasIsReadyStaticMember<B>::value (== true)
*     HasIsReadyStaticMember<Der>::value (== true)
*/
#define V_HAS_STATIC_MEMBER(_HasName, _FunctionName, _ResultType, _ParamsSignature)         \
    template<class T, class = void>                                                          \
    class Has##_HasName                                                                      \
        : public VStd::false_type                                                           \
    {                                                                                        \
    };                                                                                       \
    template<class T>                                                                        \
    class Has##_HasName<T, VStd::enable_if_t<VStd::is_invocable_r_v<                       \
            _ResultType,                                                                     \
            decltype(static_cast<_ResultType (*) _ParamsSignature>(&VStd::remove_cvref_t<T>::_FunctionName)) \
            V_FOR_EACH_UNWRAP(V_PREFIX_ARG_WITH_COMMA, _ParamsSignature)>                  \
    >>                                                                                       \
         : public VStd::true_type                                                           \
    {                                                                                        \
    };                                                                                       \
    template<class T>                                                                        \
    inline static constexpr bool Has##_HasName ## _v = Has##_HasName<T>::value;



#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_HAS_MEMBER_FUNCTION_H