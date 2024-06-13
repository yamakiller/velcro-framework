/*
 * Copyright (c) Contributors to the VelcroFramework Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_HAS_MEMBER_FUNCTION_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_HAS_MEMBER_FUNCTION_H


#include <core/std/typetraits/is_class.h>

/**
* Helper to create checkers for member function inside a class.
*
* Ex. V_HAS_MEMBER(IsReadyMember,IsReady,void,())
*     struct A {};
*     struct B
*     {
*        void IsReady();
*     };
*     HasIsReadyMember<A>::value (== false)
*     HasIsReadyMember<B>::value (== true)
*/
#define V_HAS_MEMBER(_HasName, _FunctionName, _ResultType, _ParamsSignature)                                      \
    template<bool V>                                                                                               \
    struct _HasName##ResultType { typedef VStd::true_type type; };                                                \
    template<>                                                                                                     \
    struct _HasName##ResultType<false> { typedef VStd::false_type type; };                                        \
    template<class T, bool isClass = VStd::is_class<T>::value >                                                   \
    class Has##_HasName {                                                                                          \
        typedef char Yes;                                                                                          \
        typedef long No;                                                                                           \
        template<class U, U>                                                                                       \
        struct TypeCheck;                                                                                          \
        template<class U>                                                                                          \
        struct Helper { typedef _ResultType (U::* mfp) _ParamsSignature; };                                        \
        template<class C>                                                                                          \
        static Yes ClassCheck(TypeCheck< typename Helper<C>::mfp, & C::_FunctionName>*);                           \
        template<class C>                                                                                          \
        static No  ClassCheck(...);                                                                                \
    public:                                                                                                        \
        static const bool value = (sizeof(ClassCheck< typename VStd::remove_const<T>::type >(0)) == sizeof(Yes)); \
        typedef typename _HasName##ResultType<value>::type type;                                                   \
    };                                                                                                             \
    template<class T >                                                                                             \
    struct Has##_HasName<T, false> {                                                                               \
        static const bool value = false;                                                                           \
        typedef VStd::false_type type;                                                                            \
    };

#define V_HAS_STATIC_MEMBER(_HasName, _FunctionName, _ResultType, _ParamsSignature)                               \
    template<bool V, bool dummy = true>                                                                            \
    struct _HasName##ResultType { typedef VStd::true_type type; };                                                \
    template<bool dummy>                                                                                           \
    struct _HasName##ResultType<false, dummy> { typedef VStd::false_type type; };                                 \
    template<class T, bool isClass = VStd::is_class<T>::value >                                                   \
    class Has##_HasName {                                                                                          \
        typedef char Yes;                                                                                          \
        typedef long No;                                                                                           \
        template<class U, U>                                                                                       \
        struct TypeCheck;                                                                                          \
        template<class U>                                                                                          \
        struct Helper { typedef _ResultType (* mfp) _ParamsSignature; };                                           \
        template<class C>                                                                                          \
        static Yes ClassCheck(TypeCheck< typename Helper<C>::mfp, & C::_FunctionName>*);                           \
        template<class C>                                                                                          \
        static No  ClassCheck(...);                                                                                \
    public:                                                                                                        \
        static const bool value = (sizeof(ClassCheck< typename VStd::remove_const<T>::type >(0)) == sizeof(Yes)); \
        typedef typename _HasName##ResultType<value>::type type;                                                   \
    };                                                                                                             \
    template<class T >                                                                                             \
    struct Has##_HasName<T, false> {                                                                               \
        static const bool value = false;                                                                           \
        typedef VStd::false_type type;                                                                            \
    };



#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_HAS_MEMBER_FUNCTION_H