/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef V_FRAMEWORK_CORE_RTTI_TYPE_H
#define V_FRAMEWORK_CORE_RTTI_TYPE_H

#include <vcore/std/typetraits/static_storage.h>
#include <vcore/std/typetraits/is_pointer.h>
#include <vcore/std/typetraits/is_const.h>
#include <vcore/std/typetraits/is_enum.h>
#include <vcore/std/typetraits/is_base_of.h>
#include <vcore/std/typetraits/remove_pointer.h>
#include <vcore/std/typetraits/remove_const.h>
#include <vcore/std/typetraits/remove_reference.h>
#include <vcore/std/typetraits/has_member_function.h>
#include <vcore/std/typetraits/type_identity.h>
#include <vcore/std/typetraits/void_t.h>
#include <vcore/std/function/invoke.h>
#include <vcore/math/uuid.h>
#include <vcore/math/crc.h>
#include <vcore/platform_id/platform_id.h>

#include <cstdio> // for snprintf

namespace VStd {
    template <class T>
    struct less;
    template <class T>
    struct less_equal;
    template <class T>
    struct greater;
    template <class T>
    struct greater_equal;
    template <class T>
    struct equal_to;
    template <class T>
    struct hash;
    template< class T1, class T2>
    struct pair;
    template< class T, class Allocator/* = VStd::allocator*/ >
    class vector;
    template< class T, VStd::size_t N >
    class array;
    template< class T, class Allocator/* = VStd::allocator*/ >
    class list;
    template< class T, class Allocator/* = VStd::allocator*/ >
    class forward_list;
    template<class Key, class MappedType, class Compare /*= VStd::less<Key>*/, class Allocator /*= VStd::allocator*/>
    class map;
    template<class Key, class MappedType, class Hasher /*= VStd::hash<Key>*/, class EqualKey /*= VStd::equal_to<Key>*/, class Allocator /*= VStd::allocator*/ >
    class unordered_map;
    template<class Key, class MappedType, class Hasher /* = VStd::hash<Key>*/, class EqualKey /* = VStd::equal_to<Key>*/, class Allocator /*= VStd::allocator*/ >
    class unordered_multimap;
    template <class Key, class Compare, class Allocator>
    class set;
    template<class Key, class Hasher /*= VStd::hash<Key>*/, class EqualKey /*= VStd::equal_to<Key>*/, class Allocator /*= VStd::allocator*/>
    class unordered_set;
    template<class Key, class Hasher /*= VStd::hash<Key>*/, class EqualKey /*= VStd::equal_to<Key>*/, class Allocator /*= VStd::allocator*/>
    class unordered_multiset;
    template<class T, size_t Capacity>
    class fixed_vector;
    template< class T, size_t NumberOfNodes>
    class fixed_list;
    template< class T, size_t NumberOfNodes>
    class fixed_forward_list;
    template<VStd::size_t NumBits>
    class bitset;

    template<class T>
    class intrusive_ptr;
    template<class T>
    class shared_ptr;

    template<class Element, class Traits, class Allocator>
    class basic_string;

    template<class Element>
    struct char_traits;

    template <class Element, class Traits>
    class basic_string_view;

    template <class Element, size_t MaxElementCount, class Traits>
    class basic_fixed_string;

    template<class Signature>
    class function;

    template<class T>
    class optional;

    struct monostate;

    template<class... Types>
    class variant;
}

namespace V {
    using TypeId = V::Uuid;
    struct Adl {};

    template <class T, bool IsEnum = VStd::is_enum<T>::value>
    struct VObject;

    inline namespace TypeIdResolverTags {
        //  PointerRemovedTypeId 用于查找在生成模板 typeid 时考虑指针 typeid typeid 之前创建的模板特化的 TypeId.
        //  可能 VStd::vector<V::Entity> 和 VStd::vector<V::Entity*> 具有相同的 typeid, 这会导致为其中一种类型
        //  生成不正确的序列化数据.
        //  例如: VStd::vector<V::XXX*> 的类元素应该为其元素设置的ClassElement::FLAG_POINTER, 但如果由于typeid相同,
        //  在序列化表中注册就会无法表示.
        struct PointerRemovedTypeIdTag {
        };

        // CanonicalTypeId 用于查找模板特化的 TypeId, 同时
        // 考虑到 T* 和 T 是不同的类型, 因此需要单独的 TypeId 值.
        struct CanonicalTypeIdTag {
        };

        // 对于使用专业化支持注册的模板类, 此版本将返回
        // 模板类本身的 uuid, 否则将返回规范类型.
        struct GenericTypeIdTag {
        };
    }

    namespace GenericVObject
    {
        /// Needs to match declared parameter type.
        template <template <typename...> class> constexpr bool false_v1 = false;
        template <template <VStd::size_t...> class> constexpr bool false_v2 = false;
        template <template <typename, VStd::size_t> class> constexpr bool false_v3 = false;
        template <template <typename, typename, VStd::size_t> class> constexpr bool false_v4 = false;
        template <template <typename, typename, typename, VStd::size_t> class> constexpr bool false_v5 = false;
        template <template <typename, VStd::size_t, typename> class> constexpr bool false_v6 = false;

        template<typename T>
        inline const V::TypeId& Id()
        {
            return VObject<T>::template Id<V::GenericTypeIdTag>();
        }

        template<template<typename...> class T>
        inline const V::TypeId& Id()
        {
            static_assert(false_v1<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
            static const V::TypeId _uuid = V::TypeId::CreateNull();
            return _uuid;
        }

        template<template<VStd::size_t...> class T>
        inline const V::TypeId& Id()
        {
            static_assert(false_v2<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
            static const V::TypeId _uuid = V::TypeId::CreateNull();
            return _uuid;
        }

        template<template<typename, VStd::size_t> class T>
        inline const V::TypeId& Id()
        {
            static_assert(false_v3<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
            static const V::TypeId _uuid = V::TypeId::CreateNull();
            return _uuid;
        }

        template<template<typename, typename, VStd::size_t> class T>
        inline const V::TypeId& Id()
        {
            static_assert(false_v4<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
            static const V::TypeId _uuid = V::TypeId::CreateNull();
            return _uuid;
        }

        template<template<typename, typename, typename, VStd::size_t> class T>
        inline const V::TypeId& Id()
        {
            static_assert(false_v5<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
            static const V::TypeId _uuid = V::TypeId::CreateNull();
            return _uuid;
        }

        template<template<typename, VStd::size_t, typename> class T>
        inline const V::TypeId& Id()
        {
            static_assert(false_v6<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
            static const V::TypeId _uuid = V::TypeId::CreateNull();
            return _uuid;
        }
    }

    namespace Internal {
        V_HAS_MEMBER(VObjectIntrusive, VOBJECT_Enable, void, ());

     
        template<typename T, typename = void>
        struct HasVObjectSpecialized
            : VStd::false_type
        {
        };

        template<typename T>
        struct HasVObjectSpecialized<T, VStd::enable_if_t<VStd::is_invocable_r<bool, decltype(&VObject<T>::Specialized)>::value>>
            : VStd::true_type
        {
        };

          template <class T>
        struct HasVObject
        {
            static constexpr bool value = HasVObjectSpecialized<T>::value || HasVObjectIntrusive<T>::value;
            using type = typename VStd::Utils::if_c<value, VStd::true_type, VStd::false_type>::type;
        };


        inline void VObjectSafeCat(char* destination, size_t maxDestination, const char* source)
        {
            if (!source || !destination || maxDestination == 0)
            {
                return;
            }
            size_t destinationLen = strlen(destination);
            size_t sourceLen = strlen(source);
            if (sourceLen == 0)
            {
                return;
            }
            size_t maxToCopy = maxDestination - destinationLen - 1;
            if (maxToCopy == 0)
            {
                return;
            }
            // clamp with max length
            if (sourceLen > maxToCopy)
            {
                sourceLen = maxToCopy;
            }
            destination += destinationLen;
            memcpy(destination, source, sourceLen);
            destination += sourceLen;
            *destination = 0;
        }

        template<class... Tn>
        struct AggregateTypes;

        template<class T1, class... Tn>
        struct AggregateTypes<T1, Tn...>
        {
            static void TypeName(char* buffer, size_t bufferSize)
            {
                VObjectSafeCat(buffer, bufferSize, VObject<T1>::Name());
                VObjectSafeCat(buffer, bufferSize, " ");
                AggregateTypes<Tn...>::TypeName(buffer, bufferSize);
            }

            template<typename TypeIdResolverTag = CanonicalTypeIdTag>
            static V::TypeId Id()
            {
                const V::TypeId tail = AggregateTypes<Tn...>::template Uuid<TypeIdResolverTag>();
                if (!tail.IsNull())
                {
                    // Avoid accumulating a null uuid, since it affects the result.
                    return VObject<T1>::template Id<TypeIdResolverTag>() + tail;
                }
                else
                {
                    return VObject<T1>::template Id<TypeIdResolverTag>();
                }
            }
        };

        template<>
        struct AggregateTypes<>
        {
            static void TypeName(char* buffer, size_t bufferSize)
            {
                (void)buffer; (void)bufferSize;
            }

            template<typename TypeIdResolverTag = CanonicalTypeIdTag>
            static V::TypeId Id()
            {
                return V::TypeId::CreateNull();
            }
        };

        // VS2015+/clang init them as part of static init, or interlocked (as per standard)
#if V_TRAIT_COMPILER_USE_STATIC_STORAGE_FOR_NON_POD_STATICS
        using TypeIdHolder = VStd::static_storage<V::TypeId>;
#else
        using TypeIdHolder = V::TypeId;
#endif
        inline static const V::TypeId& PointerTypeId()
        {
            static TypeIdHolder _uuid("{7074c0e8-7942-4f90-b292-950064ade25f}");
            return _uuid;
        }

        template<typename T>
        const char* GetTypeName()
        {
            return V::VObject<T>::Name();
        }

        template<int N, bool Recursion = false>
        struct NameBufferSize
        {
            static constexpr int Size = 1 + NameBufferSize<N / 10, true>::Size;
        };
        template<> struct NameBufferSize<0, false> { static constexpr const int Size = 2; };
        template<> struct NameBufferSize<0, true> { static constexpr const int Size = 1; };

        template<VStd::size_t N>
        const char* GetTypeName()
        {
            static char buffer[NameBufferSize<N>::Size] = { 0 };
            if (buffer[0] == 0)
            {
                v_snprintf(buffer, V_ARRAY_SIZE(buffer), "%zu", N);
            }
            return buffer;
        }

        template<typename T, typename TypeIdResolverTag>
        const V::TypeId& GetTypeId()
        {
            return V::VObject<T>::template Uuid<TypeIdResolverTag>();
        }

        template<VStd::size_t N, typename>
        const V::TypeId& GetTypeId()
        {
            static V::TypeId uuid = V::TypeId::CreateName(GetTypeName<N>());
            return uuid;
        }
    } // namespace Internal

    enum class TypeTraits : V::u64
    {
        //! Stores the result of the std::is_signed check for a registered type
        is_signed = 0x01,
        //! Stores the result of the std::is_unsigned check for a registered type
        //! NOTE: While this might seem to be redundant due to is_signed, it is NOT
        //! Only numeric types have a concept of signedness
        is_unsigned = 0x02,
        //! Stores the result of the std::is_enum check
        is_enum = 0x04
    };

    V_DEFINE_ENUM_BITWISE_OPERATORS(V::TypeTraits);

  

    // 非Enum数据类型
    template<class T>
    struct VObject<T, false>
    {
        static const char* Name() {
            static_assert(V::Internal::HasVObjectIntrusive<T>::value, 
            "You should use VOBJECT in your class/struct, or use VOBJECT_SPECIALIZE() externally." 
            "Make sure to include the header containing this information (usually your class header).");
            return T::VOBJECT_Name();
        }

        static const V::TypeId& Id()
        {
            static_assert(V::Internal::HasVObjectIntrusive<T>::value,
                "You should use VOBJECT in your class/struct, or use VOBJECT_SPECIALIZE() externally. "
                "Make sure to include the header containing this information (usually your class header).");
            return T::VOBJECT_Id();
        }

        static constexpr TypeTraits GetTypeTraits()
        {
            TypeTraits typeTraits{};
            // Track the C++ type traits required by the SerializeContext
            typeTraits |= std::is_signed<T>::value ? TypeTraits::is_signed : typeTraits;
            typeTraits |= std::is_unsigned<T>::value ? TypeTraits::is_unsigned : typeTraits;
            return typeTraits;
        }

        static constexpr size_t Size()
        {
            return sizeof(T);
        }
    };
    
    // Enum 数据类型
    template <class T>
    struct VObject<T, true>
    {
        typedef typename VStd::remove_enum<T>::type UnderlyingType;
        static const char* Name() { return nullptr; }
   

        static const V::TypeId& Id() { static V::TypeId nullUuid = V::TypeId::CreateNull(); return nullUuid; }
        static constexpr TypeTraits GetTypeTraits()
        {
            TypeTraits typeTraits{};
            // Track the C++ type traits required by the SerializeContext
            typeTraits |= std::is_signed<UnderlyingType>::value ? TypeTraits::is_signed : typeTraits;
            typeTraits |= std::is_unsigned<UnderlyingType>::value ? TypeTraits::is_unsigned : typeTraits;
            typeTraits |= TypeTraits::is_enum;
            return typeTraits;
        }

        static constexpr size_t Size()
        {
            return sizeof(T);
        }
    };

     template<class T, class U>
    inline bool operator==(VObject<T> const& lhs, VObject<U> const& rhs)
    {
        return lhs.Id() == rhs.Id();
    }

    template<class T, class U>
    inline bool operator!=(VObject<T> const& lhs, VObject<U> const& rhs)
    {
        return lhs.Id() != rhs.Id();
    }

    namespace Internal
    {
        // Sizeof helper to deal with incomplete types and the void type
        template<typename T, typename = void>
        struct VObjectSizeOf
        {
            static constexpr size_t Size()
            {
                return 0;
            }
        };

        // In this case that sizeof(T) is a compilable, the type is complete
        template<typename T>
        struct VObjectSizeOf<T, VStd::void_t<decltype(sizeof(T))>>
        {
            static constexpr size_t Size()
            {
                return sizeof(T);
            }
        };
    }

    // VObject specialization helper for non intrusive TypeInfo
    #define VOBJECT_INFO_INTERNAL_SPECIALIZE(_ClassName, _ClassUuid)                 \
            template<>                                                               \
            struct VObject<_ClassName, VStd::is_enum<_ClassName>::value>             \
            {                                                                        \
                static const char* Name() { return #_ClassName; }                    \
                template<typename TypeIdResolverTag = CanonicalTypeIdTag>            \
                static const V::TypeId& Id() {                                       \
                    static V::Internal::TypeIdHolder _uuid(_ClassUuid);              \
                    return _uuid;                                                    \
                }                                                                    \
                static constexpr TypeTraits GetTypeTraits()                          \
                {                                                                    \
                    TypeTraits typeTraits{};                                         \
                    typeTraits |= std::is_signed<VStd::remove_enum_t<_ClassName>>::value ? TypeTraits::is_signed : typeTraits;     \
                    typeTraits |= std::is_unsigned<VStd::remove_enum_t<_ClassName>>::value ? TypeTraits::is_unsigned : typeTraits; \
                    typeTraits |= std::is_enum<_ClassName>::value ? TypeTraits::is_enum: typeTraits;                               \
                    return typeTraits;                                        \
                }                                                             \
                static constexpr size_t Size()                                \
                {                                                             \
                    return V::Internal::VObjectSizeOf<_ClassName>::Size();   \
                }                                                             \
                static bool Specialized() { return true; }                    \
            };
    // specialize 函数
    template<class R, class... Args>
    struct VObject<R(Args...), false>
    {
        static const char* Name()
        {
            static char typeName[64] = { 0 };
            if (typeName[0] == 0)
            {
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), "{");
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), V::VObject<R>::Name());
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), "(");
                V::Internal::AggregateTypes<Args...>::TypeName(typeName, V_ARRAY_SIZE(typeName));
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), ")}");
            }
            return typeName;
        }
        template<typename TypeIdResolverTag = CanonicalTypeIdTag>
        static const V::TypeId& Id() {
            static V::Internal::TypeIdHolder _uuid(V::Internal::AggregateTypes<R, Args...>::template Uuid<>(TypeIdResolverTag));
            return _uuid;
        }
        static constexpr TypeTraits GetTypeTraits()
        {
            TypeTraits typeTraits{};
            typeTraits |= std::is_signed<R(Args...)>::value ? TypeTraits::is_signed : typeTraits;
            typeTraits |= std::is_unsigned<R(Args...)>::value ? TypeTraits::is_unsigned : typeTraits;
            return typeTraits;
        }
        static constexpr size_t Size()
        {
            return sizeof(R(Args...));
        }
    };

    // specialize 成员函数指针
    template<class R, class C, class... Args>
    struct VObject<R(C::*)(Args...), false> {
        static const char* Name() {
            static char typeName[128] = {0};
            if (typeName[0] == 0) {
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), "{");
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), V::VObject<R>::Name());
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), "(");
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), V::VObject<C>::Name());
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), "::*)");
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), "(");
                V::Internal::AggregateTypes<Args...>::TypeName(typeName, V_ARRAY_SIZE(typeName));
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), ")}");
            }
            return typeName;
        }
        template<typename TypeIdResolverTag = CannonicalTypeIdTag>
        static const V::TypeId& Id() {
            static V::Internal::TypeIdHolder _uuid(V::Internal::AggregateTypes<R, C, Args...>::template Uuid<TypeIdResolverTag>());
            return _uuid;
        }
        static constexpr TypeTraits GetTypeTraits()
        {
            TypeTraits typeTraits{};
            typeTraits |= std::is_signed<R(C::*)(Args...)>::value ? TypeTraits::is_signed : typeTraits;
            typeTraits |= std::is_unsigned<R(C::*)(Args...)>::value ? TypeTraits::is_unsigned : typeTraits;
            return typeTraits;
        }
        static constexpr size_t Size()
        {
            return sizeof(R(C::*)(Args...));
        }
    };

    // specialize for const member function pointers
    template<class R, class C, class... Args>
    struct VObject<R(C::*)(Args...) const, false>
        : VObject<R(C::*)(Args...), false> {};

    // specialize for member data pointers
    template<class R, class C>
    struct VObject<R C::*, false>
    {
        static const char* Name()
        {
            static char typeName[64] = { 0 };
            if (typeName[0] == 0)
            {
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), "{");
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), V::VObject<R>::Name());
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), " ");
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), V::VObject<C>::Name());
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), "::*}");
            }
            return typeName;
        }
        template<typename TypeIdResolverTag = CanonicalTypeIdTag>
        static const V::TypeId& Id()
        {
            static V::Internal::TypeIdHolder _uuid(V::Internal::AggregateTypes<R, C>::template Uuid<TypeIdResolverTag>());
            return _uuid;
        }
        static constexpr TypeTraits GetTypeTraits()
        {
            TypeTraits typeTraits{};
            typeTraits |= std::is_signed<R C::*>::value ? TypeTraits::is_signed : typeTraits;
            typeTraits |= std::is_unsigned<R C::*>::value ? TypeTraits::is_unsigned : typeTraits;
            return typeTraits;
        }
        static constexpr size_t Size()
        {
            return sizeof(R C::*);
        }
    };

    #define VOBJECT_INTERNAL_SPECIALIZE_CV(_T1, _Specialization, _NamePrefix, _NameSuffix)                       \
    template<class _T1>                                                                                          \
    struct VObject<_Specialization, false> {                                                                     \
        static const char* Name() {                                                                              \
            static char typeName[64] = { 0 };                                                                    \
            if (typeName[0] == 0) {                                                                              \
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), _NamePrefix);                      \
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), V::VObject<_T1>::Name());          \
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), _NameSuffix);                      \
            }                                                                                                    \
            return typeName;                                                                                     \
        }                                                                                                        \
        /*                                                                                                       \
        * By default the specialization for pointer types defaults to PointerRemovedTypeIdTag                    \
        * This allows the current use of VObject<T*>::Uuid to still return the typeid of type                    \
        */                                                                                                       \
        template<typename TypeIdResolverTag = PointerRemovedTypeIdTag>                                           \
        static const V::TypeId& Id() {                                                                           \
            static V::Internal::TypeIdHolder _uuid(                                                              \
                !VStd::is_same<TypeIdResolverTag, PointerRemovedTypeIdTag>::value                                \
                && VStd::is_pointer<_Specialization>::value ?                                                    \
                V::VObject<_T1>::template Id<TypeIdResolverTag>() + V::Internal::PointerTypeId() : V::VObject<_T1>::template Id<TypeIdResolverTag>()); \
            return _uuid;                                                                                        \
        }                                                                                                        \
        static constexpr TypeTraits GetTypeTraits()                                                              \
        {                                                                                                        \
            TypeTraits typeTraits{};                                                                             \
            typeTraits |= std::is_signed<_Specialization>::value ? TypeTraits::is_signed : typeTraits;           \
            typeTraits |= std::is_unsigned<_Specialization>::value ? TypeTraits::is_unsigned : typeTraits;       \
            return typeTraits;                                                                                   \
        }                                                                                                        \
        static constexpr size_t Size()                                                                           \
        {                                                                                                        \
            return sizeof(_Specialization);                                                                      \
        }                                                                                                        \
        static bool Specialized() { return true; }                                                               \
    };

    // Helper macros to generically specialize template types
    #define  VOBJECT_INTERNAL_VARIATION_GENERIC(_Generic, _Uuid)                                                  \
    namespace GenericVObject {                                                                                    \
        template<>                                                                                                \
        inline const V::TypeId& Id<_Generic>(){ static V::Internal::TypeIdHolder _uuid(_Uuid); return _uuid; }    \
    }

    #define VOBJECT_INTERNAL_TYPENAME__TYPE typename
    #define VOBJECT_INTERNAL_TYPENAME__ARG(A) A
    #define VOBJECT_INTERNAL_TYPENAME__ID(Tag, A) V::Internal::GetTypeId< A , Tag >()
    #define VOBJECT_INTERNAL_TYPENAME__NAME(A) V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), V::Internal::GetTypeName< A >())

    #define VOBJECT_INTERNAL_CLASS__TYPE class
    #define VOBJECT_INTERNAL_CLASS__ARG(A) A
    #define VOBJECT_INTERNAL_CLASS__ID(Tag, A) V::Internal::GetTypeId< A , Tag >()
    #define VOBJECT_INTERNAL_CLASS__NAME(A) V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), V::Internal::GetTypeName< A >())


    #define VOBJECT_INTERNAL_TYPENAME_VARARGS__TYPE typename...
    #define VOBJECT_INTERNAL_TYPENAME_VARARGS__ARG(A) A...
    #define VOBJECT_INTERNAL_TYPENAME_VARARGS__ID(Tag, A) V::Internal::AggregateTypes< A... >::template Id< Tag >()
    #define VOBJECT_INTERNAL_TYPENAME_VARARGS__NAME(A) V::Internal::AggregateTypes< A... >::TypeName(typeName, V_ARRAY_SIZE(typeName))

    #define VOBJECT_INTERNAL_CLASS_VARARGS__TYPE class...
    #define VOBJECT_INTERNAL_CLASS_VARARGS__ARG(A) A...
    #define VOBJECT_INTERNAL_CLASS_VARARGS__ID(Tag, A) V::Internal::AggregateTypes< A... >::template Id< Tag >()
    #define VOBJECT_INTERNAL_CLASS_VARARGS__NAME(A) V::Internal::AggregateTypes< A... >::TypeName(typeName, V_ARRAY_SIZE(typeName));

    // Once C++17 has been introduced size_t can be replaced with auto for all integer non-type arguments
    #define VOBJECT_INTERNAL_AUTO__TYPE VStd::size_t
    #define VOBJECT_INTERNAL_AUTO__ARG(A) A
    #define VOBJECT_INTERNAL_AUTO__ID(Tag, A) V::Internal::GetTypeId< A , Tag >()
    #define VOBJECT_INTERNAL_AUTO__NAME(A) V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), V::Internal::GetTypeName< A >())

    #define VOBJECT_INTERNAL_EXPAND_I(NAME, TARGET)     NAME##__##TARGET
    #define VOBJECT_INTERNAL_EXPAND(NAME, TARGET)  VOBJECT_INTERNAL_EXPAND_I(NAME, TARGET)


    #define VOBJECT_INTERNAL_TEMPLATE_TYPE_EXPANSION_1(_1)                                                                                          VOBJECT_INTERNAL_EXPAND(_1, TYPE) T1
    #define VOBJECT_INTERNAL_TEMPLATE_TYPE_EXPANSION_2(_1, _2)                      VOBJECT_INTERNAL_TEMPLATE_TYPE_EXPANSION_1(_1),                 VOBJECT_INTERNAL_EXPAND(_2, TYPE) T2
    #define VOBJECT_INTERNAL_TEMPLATE_TYPE_EXPANSION_3(_1, _2, _3)                  VOBJECT_INTERNAL_TEMPLATE_TYPE_EXPANSION_2(_1, _2),             VOBJECT_INTERNAL_EXPAND(_3, TYPE) T3
    #define VOBJECT_INTERNAL_TEMPLATE_TYPE_EXPANSION_4(_1, _2, _3, _4)              VOBJECT_INTERNAL_TEMPLATE_TYPE_EXPANSION_3(_1, _2, _3),         VOBJECT_INTERNAL_EXPAND(_4, TYPE) T4
    #define VOBJECT_INTERNAL_TEMPLATE_TYPE_EXPANSION_5(_1, _2, _3, _4, _5)          VOBJECT_INTERNAL_TEMPLATE_TYPE_EXPANSION_4(_1, _2, _3, _4),     VOBJECT_INTERNAL_EXPAND(_5, TYPE) T5
    #define VOBJECT_INTERNAL_TEMPLATE_TYPE_EXPANSION(...) V_MACRO_SPECIALIZE(VOBJECT_INTERNAL_TEMPLATE_TYPE_EXPANSION_, V_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

    #define VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_1(_1)                                                                                      VOBJECT_INTERNAL_EXPAND(_1, ARG)(T1)
    #define VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_2(_1, _2)             VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_1(_1),                  VOBJECT_INTERNAL_EXPAND(_2, ARG)(T2)
    #define VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_3(_1, _2, _3)         VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_2(_1, _2),              VOBJECT_INTERNAL_EXPAND(_3, ARG)(T3)
    #define VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_4(_1, _2, _3, _4)     VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_3(_1, _2, _3),          VOBJECT_INTERNAL_EXPAND(_4, ARG)(T4)
    #define VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_5(_1, _2, _3, _4, _5) VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_4(_1, _2, _3, _4),      VOBJECT_INTERNAL_EXPAND(_5, ARG)(T5)
    #define VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(...) V_MACRO_SPECIALIZE(VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION_, V_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

    #define VOBJECT_INTERNAL_TEMPLATE_NAME_EXPANSION_1(_1)                                                                                                                                                          VOBJECT_INTERNAL_EXPAND(_1, NAME)(T1);
    #define VOBJECT_INTERNAL_TEMPLATE_NAME_EXPANSION_2(_1, _2)                 VOBJECT_INTERNAL_TEMPLATE_NAME_EXPANSION_1(_1)             V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), ", ");      VOBJECT_INTERNAL_EXPAND(_2, NAME)(T2);
    #define VOBJECT_INTERNAL_TEMPLATE_NAME_EXPANSION_3(_1, _2, _3)             VOBJECT_INTERNAL_TEMPLATE_NAME_EXPANSION_2(_1, _2)         V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), ", ");      VOBJECT_INTERNAL_EXPAND(_3, NAME)(T3);
    #define VOBJECT_INTERNAL_TEMPLATE_NAME_EXPANSION_4(_1, _2, _3, _4)         VOBJECT_INTERNAL_TEMPLATE_NAME_EXPANSION_3(_1, _2, _3)     V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), ", ");      VOBJECT_INTERNAL_EXPAND(_4, NAME)(T4);
    #define VOBJECT_INTERNAL_TEMPLATE_NAME_EXPANSION_5(_1, _2, _3, _4, _5)     VOBJECT_INTERNAL_TEMPLATE_NAME_EXPANSION_4(_1, _2, _3, _4) V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), ", ");      VOBJECT_INTERNAL_EXPAND(_5, NAME)(T5);
    #define VOBJECT_INTERNAL_TEMPLATE_NAME_EXPANSION(...) V_MACRO_SPECIALIZE(VOBJECT_INTERNAL_TEMPLATE_NAME_EXPANSION_, V_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

    #define VOBJECT_INTERNAL_TEMPLATE_ID_EXPANSION_1(Tag, _1)                                                                                  VOBJECT_INTERNAL_EXPAND(_1, ID)(Tag, T1)
    #define VOBJECT_INTERNAL_TEMPLATE_ID_EXPANSION_2(Tag, _1, _2)             (VOBJECT_INTERNAL_TEMPLATE_ID_EXPANSION_1(Tag, _1) +             VOBJECT_INTERNAL_EXPAND(_2, ID)(Tag, T2))
    #define VOBJECT_INTERNAL_TEMPLATE_ID_EXPANSION_3(Tag, _1, _2, _3)         (VOBJECT_INTERNAL_TEMPLATE_ID_EXPANSION_2(Tag, _1, _2) +         VOBJECT_INTERNAL_EXPAND(_3, ID)(Tag, T3))
    #define VOBJECT_INTERNAL_TEMPLATE_ID_EXPANSION_4(Tag, _1, _2, _3, _4)     (VOBJECT_INTERNAL_TEMPLATE_ID_EXPANSION_3(Tag, _1, _2, _3) +     VOBJECT_INTERNAL_EXPAND(_4, ID)(Tag, T4))
    #define VOBJECT_INTERNAL_TEMPLATE_ID_EXPANSION_5(Tag, _1, _2, _3, _4, _5) (VOBJECT_INTERNAL_TEMPLATE_ID_EXPANSION_4(Tag, _1, _2, _3, _4) + VOBJECT_INTERNAL_EXPAND(_5, ID)(Tag, T5))
    #define VOBJECT_INTERNAL_TEMPLATE_ID_EXPANSION(Tag, ...) V_MACRO_SPECIALIZE(VOBJECT_INTERNAL_TEMPLATE_ID_EXPANSION_, V_VA_NUM_ARGS(__VA_ARGS__), (Tag, __VA_ARGS__))

    #define VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_PREFIX_ID(_Specialization, _Name, _ClassUuid, ...)                \
    VOBJECT_INTERNAL_VARIATION_GENERIC(_Specialization, _ClassUuid)                                                 \
    template<VOBJECT_INTERNAL_TEMPLATE_TYPE_EXPANSION(__VA_ARGS__)>                                                 \
    struct VObject<_Specialization<VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>, false> {             \
        static const char* Name() {                                                                                 \
            static char typeName[128] = { 0 };                                                                      \
            if (typeName[0] == 0) {                                                                                 \
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), _Name "<");                           \
                VOBJECT_INTERNAL_TEMPLATE_NAME_EXPANSION(__VA_ARGS__)                                               \
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), ">");                                 \
            }                                                                                                       \
            return typeName;                                                                                        \
        }                                                                                                           \
    private:                                                                                                        \
        static const V::TypeId& IdTag(V::TypeIdResolverTags::CanonicalTypeIdTag)                                    \
        {                                                                                                           \
            static V::Internal::TypeIdHolder _uuid(                                                                 \
                V::TypeId(_ClassUuid) +                                                                             \
                VOBJECT_INTERNAL_TEMPLATE_ID_EXPANSION(V::TypeIdResolverTags::CanonicalTypeIdTag, __VA_ARGS__));    \
            return _uuid;                                                                                           \
        }                                                                                                           \
        static const V::TypeId& IdTag(V::TypeIdResolverTags::PointerRemovedTypeIdTag)                               \
        {                                                                                                           \
            static V::Internal::TypeIdHolder _uuid(                                                                 \
                V::TypeId(_ClassUuid) +                                                                             \
                VOBJECT_INTERNAL_TEMPLATE_ID_EXPANSION(V::TypeIdResolverTags::PointerRemovedTypeIdTag, __VA_ARGS__)); \
            return _uuid;                                                                                           \
        }                                                                                                           \
        static const V::TypeId& IdTag(V::TypeIdResolverTags::GenericTypeIdTag)                                      \
        {                                                                                                           \
            return GenericVObject::Id<_Specialization>();                                                           \
        }                                                                                                           \
    public:                                                                                                         \
        template<typename Tag = V::TypeIdResolverTags::CanonicalTypeIdTag>                                          \
        static const V::TypeId& Id()                                                                                \
        {                                                                                                           \
            return IdTag(Tag{});                                                                                    \
        }                                                                                                           \
        static constexpr TypeTraits GetTypeTraits()                                                                 \
        {                                                                                                           \
            TypeTraits typeTraits{};                                                                                \
            typeTraits |= std::is_signed<_Specialization<VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>>::value ? TypeTraits::is_signed : typeTraits; \
            typeTraits |= std::is_unsigned<_Specialization<VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>>::value ? TypeTraits::is_unsigned : typeTraits; \
            return typeTraits;                                                                                      \
        }                                                                                                           \
        static constexpr size_t Size()                                                                              \
        {                                                                                                           \
            return sizeof(_Specialization<VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>);              \
        }                                                                                                           \
        static bool Specialized() { return true; }                                                                  \
    }


    #define VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(_Specialization, _ClassUuid, ...)                      \
    VOBJECT_INTERNAL_VARIATION_GENERIC(_Specialization, _ClassUuid)                                                 \
    template<VOBJECT_INTERNAL_TEMPLATE_TYPE_EXPANSION(__VA_ARGS__)>                                                 \
    struct VObject<_Specialization<VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>, false> {             \
        static const char* Name() {                                                                                 \
            static char typeName[128] = { 0 };                                                                      \
            if (typeName[0] == 0) {                                                                                 \
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), #_Specialization "<");                \
                VOBJECT_INTERNAL_TEMPLATE_NAME_EXPANSION(__VA_ARGS__)                                               \
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), ">");                                 \
            }                                                                                                       \
            return typeName;                                                                                        \
        }                                                                                                           \
    private:                                                                                                        \
        static const V::TypeId& IdTag(V::TypeIdResolverTags::CanonicalTypeIdTag)                                    \
        {                                                                                                           \
            static V::Internal::TypeIdHolder   _uuid(                                                               \
                VOBJECT_INTERNAL_TEMPLATE_ID_EXPANSION(V::TypeIdResolverTags::CanonicalTypeIdTag, __VA_ARGS__)      \
                + V::TypeId(_ClassUuid));                                                                           \
            return _uuid;                                                                                           \
        }                                                                                                           \
        static const V::TypeId& IdTag(V::TypeIdResolverTags::PointerRemovedTypeIdTag)                               \
        {                                                                                                           \
            static V::Internal::TypeIdHolder _uuid(                                                                 \
                VOBJECT_INTERNAL_TEMPLATE_ID_EXPANSION(V::TypeIdResolverTags::PointerRemovedTypeIdTag, __VA_ARGS__) \
                + V::TypeId(_ClassUuid));                                                                           \
            return _uuid;                                                                                           \
        }                                                                                                           \
        static const V::TypeId& IdTag(V::TypeIdResolverTags::GenericTypeIdTag)                                      \
        {                                                                                                           \
            return GenericVObject::Id<_Specialization>();                                                           \
        }                                                                                                           \
    public:                                                                                                         \
        template<typename Tag = V::TypeIdResolverTags::CanonicalTypeIdTag>                                          \
        static const V::TypeId& Id()                                                                                \
        {                                                                                                           \
            return IdTag(Tag{});                                                                                    \
        }                                                                                                           \
        static constexpr TypeTraits GetTypeTraits()                                                                 \
        {                                                                                                           \
            TypeTraits typeTraits{};                                                                                \
            typeTraits |= std::is_signed<_Specialization<VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>>::value ? TypeTraits::is_signed : typeTraits; \
            typeTraits |= std::is_unsigned<_Specialization<VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>>::value ? TypeTraits::is_unsigned : typeTraits; \
            return typeTraits;                                                                                      \
        }                                                                                                           \
        static constexpr size_t Size()                                                                              \
        {                                                                                                           \
            return sizeof(_Specialization<VOBJECT_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__)>);              \
        }                                                                                                           \
        static bool Specialized() { return true; }                                                                  \
    }

    #define VOBJECT_INTERNAL_FUNCTION_VARIATION_SPECIALIZATION(_Specialization, _ClassUuid)\
    VOBJECT_INTERNAL_VARIATION_GENERIC(_Specialization, _ClassUuid) \
    template<typename R, typename... Args> \
    struct VObject<_Specialization<R(Args...)>, false> {\
        static const char* Name() { \
            static char typeName[128] = { 0 }; \
            if (typeName[0] == 0) { \
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), #_Specialization "<"); \
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), V::VObject<R>::Name()); \
                V::Internal::AggregateTypes<Args...>::TypeName(typeName, V_ARRAY_SIZE(typeName)); \
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), ">"); \
            }\
            return typeName; \
        } \
    private: \
        static const V::TypeId& IdTag(V::TypeIdResolverTags::CanonicalTypeIdTag) \
        { \
            static V::Internal::TypeIdHolder _uuid( \
                V::Uuid(_ClassUuid) + V::VObject<R>::template Id<V::TypeIdResolverTags::CanonicalTypeIdTag>() + \
                V::Internal::AggregateTypes<Args...>::template Id<V::TypeIdResolverTags::CanonicalTypeIdTag>()); \
            return _uuid; \
        } \
        static const V::TypeId& IdTag(V::TypeIdResolverTags::PointerRemovedTypeIdTag) \
        { \
            static V::Internal::TypeIdHolder _uuid( \
                V::Uuid(_ClassUuid) + V::VObject<R>::template Id<V::TypeIdResolverTags::PointerRemovedTypeIdTag>() + \
                V::Internal::AggregateTypes<Args...>::template Id<V::TypeIdResolverTags::PointerRemovedTypeIdTag>()); \
            return _uuid; \
        } \
        static const V::TypeId& IdTag(V::TypeIdResolverTags::GenericTypeIdTag) \
        { \
            return GenericVObject::Id<_Specialization>(); \
        } \
    public: \
        template<typename Tag = V::TypeIdResolverTags::CanonicalTypeIdTag> \
        static const V::TypeId& Id() \
        { \
            return IdTag(Tag{}); \
        } \
        static constexpr TypeTraits GetTypeTraits() \
        { \
            TypeTraits typeTraits{}; \
            typeTraits |= std::is_signed<_Specialization<R(Args...)>>::value ? TypeTraits::is_signed : typeTraits; \
            typeTraits |= std::is_unsigned<_Specialization<R(Args...)>>::value ? TypeTraits::is_unsigned : typeTraits; \
            return typeTraits; \
        } \
        static constexpr size_t Size() \
        { \
            return sizeof(_Specialization<R(Args...)>); \
        } \
        static bool Specialized() { return true; } \
    }

    /* This version of VOBJECT_INTERNAL_VARIATION_SPECIALIZATION_2 only uses the first argument for Id generation purposes */
    #define VOBJECT_INTERNAL_VARIATION_SPECIALIZATION_2_CONCAT_1(_Specialization, _AddUuid, _T1, _T2)                \
    VOBJECT_INTERNAL_VARIATION_GENERIC(_Specialization, _AddUuid)                                                    \
    template<class _T1, class _T2>                                                                                   \
    struct VObject<_Specialization<_T1, _T2>, false> {                                                               \
        static const char* Name() {                                                                                  \
            static char typeName[128] = { 0 };                                                                       \
            if (typeName[0] == 0) {                                                                                  \
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), #_Specialization "<");                 \
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), V::VObject<_T1>::Name());              \
                V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), ">");                                  \
            }                                                                                                        \
            return typeName;                                                                                         \
        }                                                                                                            \
    private:                                                                                                         \
        static const V::TypeId& IdTag(V::TypeIdResolverTags::CanonicalTypeIdTag)                                     \
        {                                                                                                            \
            static V::Internal::TypeIdHolder _uuid(                                                                  \
                V::VObject<_T1>::template Id<V::TypeIdResolverTags::CanonicalTypeIdTag>() +                          \
                V::TypeId(_AddUuid));                                                                                \
            return _uuid;                                                                                            \
        }                                                                                                            \
        static const V::TypeId& IdTag(V::TypeIdResolverTags::PointerRemovedTypeIdTag)                                \
        {                                                                                                            \
            static V::Internal::TypeIdHolder _uuid(                                                                  \
                V::VObject<_T1>::template Id<V::TypeIdResolverTags::PointerRemovedTypeIdTag>() +                     \
                V::TypeId(_AddUuid));                                                                                \
            return _uuid;                                                                                            \
        }                                                                                                            \
        static const V::TypeId& IdTag(V::TypeIdResolverTags::GenericTypeIdTag)                                       \
        {                                                                                                            \
            return GenericVObject::Id<_Specialization>();                                                            \
        }                                                                                                            \
    public:                                                                                                          \
        template<typename Tag = V::TypeIdResolverTags::CanonicalTypeIdTag>                                           \
        static const V::TypeId& Id()                                                                                 \
        {                                                                                                            \
            return IdTag(Tag{});                                                                                     \
        }                                                                                                            \
        static constexpr TypeTraits GetTypeTraits()                                                                  \
        {                                                                                                            \
            TypeTraits typeTraits{};                                                                                 \
            typeTraits |= std::is_signed<_Specialization<_T1, _T2>>::value ? TypeTraits::is_signed : typeTraits;     \
            typeTraits |= std::is_unsigned<_Specialization<_T1, _T2>>::value ? TypeTraits::is_unsigned : typeTraits; \
            return typeTraits;                                                                                       \
        }                                                                                                            \
        static constexpr size_t Size()                                                                               \
        {                                                                                                            \
            return sizeof(_Specialization<_T1, _T2>);                                                                \
        }                                                                                                            \
        static bool Specialized() { return true; }                                                                   \
    }
} // namespace V 


/*#define VOBJECT(_ClassName, _ClassUuid)                                                               \
    void  VOBJECT_Enable() {}                                                                         \
    static const char* VOBJECT_Name() { return #_ClassName; }                                         \
    static const V::TypeId& VOBJECT_Id() { static V::TypeId _uuid(_ClassUuid); return _uuid; }        \
    VOBJECT_INFO_INTERNAL_SPECIALIZE(_ClassName, _ClassUuid)*/



namespace V {
    VOBJECT_INFO_INTERNAL_SPECIALIZE(char, "{1e7e0bdc-e68f-4fcb-ad04-cbd9ea6d4694}");
    VOBJECT_INFO_INTERNAL_SPECIALIZE(int,  "{ad45de19-744a-44f7-8961-422029b86879}");
    VOBJECT_INFO_INTERNAL_SPECIALIZE(long, "{47e78e95-e864-46a2-9228-a1dad678fdd1}");
    VOBJECT_INFO_INTERNAL_SPECIALIZE(V::s8, "{3e779afc-a675-47bb-8134-50ce605fa874}");
    VOBJECT_INFO_INTERNAL_SPECIALIZE(short, "{4ee31435-8299-4b9b-90a4-9043be75103d}");
    VOBJECT_INFO_INTERNAL_SPECIALIZE(V::s64, "{faece31e-390a-46f5-8123-fe3e00a53166}");
    VOBJECT_INFO_INTERNAL_SPECIALIZE(unsigned char, "{ff197fee-7514-452a-a4b5-cf5759ef6208}");
    VOBJECT_INFO_INTERNAL_SPECIALIZE(unsigned short, "{2b7a7dd7-164e-4749-9893-5a79ff34fdeb}");
    VOBJECT_INFO_INTERNAL_SPECIALIZE(unsigned int, "{1b2dba6d-af5e-43fa-a3bd-801e57483e0c}");
    VOBJECT_INFO_INTERNAL_SPECIALIZE(unsigned long, "{a59aafe3-4486-4d19-a08e-aa3d16379887}");
    VOBJECT_INFO_INTERNAL_SPECIALIZE(V::u64, "{1c967c84-e875-4235-8257-c15754bb7678}");
    VOBJECT_INFO_INTERNAL_SPECIALIZE(float, "{bdbfaf1e-c259-4d0a-aae0-7b68e07b7698}");
    VOBJECT_INFO_INTERNAL_SPECIALIZE(double, "{9b828f8f-26a6-4f02-b549-2eb1b9f15bc6}");
    VOBJECT_INFO_INTERNAL_SPECIALIZE(bool, "{00362454-7079-4b85-885a-a29f9f634615}");
    VOBJECT_INFO_INTERNAL_SPECIALIZE(V::Uuid, "{8e99ee80-af97-4767-a5c3-70a46793c700}");
    VOBJECT_INFO_INTERNAL_SPECIALIZE(void, "{6c20c236-3bbe-4c44-84c5-20acdcd69531}");
    VOBJECT_INFO_INTERNAL_SPECIALIZE(Crc32, "{bbc4eff2-2079-41cd-ac56-ecf1f89a029c}");
    VOBJECT_INFO_INTERNAL_SPECIALIZE(PlatformID, "{4feafc8b-b03d-47c2-86f7-a318cb7d9c09}");

    VOBJECT_INTERNAL_SPECIALIZE_CV(T, T*, "", "*");
    VOBJECT_INTERNAL_SPECIALIZE_CV(T, T &, "", "&");
    VOBJECT_INTERNAL_SPECIALIZE_CV(T, T &&, "", "&&");
    VOBJECT_INTERNAL_SPECIALIZE_CV(T, const T*, "const ", "*");
    VOBJECT_INTERNAL_SPECIALIZE_CV(T, const T&, "const ", "&");
    VOBJECT_INTERNAL_SPECIALIZE_CV(T, const T&&, "const ", "&&");
    VOBJECT_INTERNAL_SPECIALIZE_CV(T, const T, "const ", "");


    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::less,           "{49fb7049-8072-44f6-ab37-4be6d0890bef}", VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::less_equal,     "{232db2eb-35e9-4a48-900f-a503d0e0b227}", VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::greater,        "{a67c99cd-2acc-4e30-a86e-d2d7f66a1c81}", VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::greater_equal,  "{b23dcb47-5adc-4a7e-8b97-3e81f3f4f978}", VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::equal_to,       "{b46e1e2f-ae1b-47e3-a74a-580b3db3893c}", VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::pair,           "{7ac10046-39bb-4341-a768-6b9749aa2e8e}", VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::vector,         "{eb36f863-8200-4384-9859-6f2e0824259c}", VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::list,           "{31b58989-1c82-4f63-bc43-1f927ad8a38c}", VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME);

    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::forward_list,        "{d7a2baca-8b4a-43cc-abe4-a7bbac1d30fb}", VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::set,                 "{7d88d8b0-d848-4248-9293-f188c6c6cb88}", VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::unordered_set,       "{122b56ea-4574-4ba5-b6e3-80fc01346890}", VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::unordered_multiset,  "{25d022fe-d9e4-4191-875e-45cbbe939d90}", VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::map,                 "{d8b33e17-9474-442a-a3d3-b609a1d176f4}", VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::unordered_map,       "{680f18b1-8d20-4e7a-8b81-879a3d82ae86}", VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::unordered_multimap,  "{22ca2cb0-8605-4e86-8aa0-245803fb9a41}", VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::shared_ptr,          "{259a428c-ebc7-4905-b195-c65918635298}", VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::intrusive_ptr,       "{9920ca18-108e-4231-a4c0-38315df4a874}", VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::optional,            "{fd49de47-e823-43d0-8ade-a99a1d121a9e}", VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::basic_string,        "{a56a11c4-493f-446e-8567-e5acc877d17a}", VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::char_traits,         "{9df5f4b3-fc8d-40ac-aec2-bf05f4c8c521}", VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::basic_string_view,   "{a90eb794-2758-4dda-b563-e0a623ad65c4}", VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_TYPENAME);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::fixed_vector,        "{94c90a15-0052-460d-9f11-bb3843e17a60}", VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_AUTO);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::fixed_list,          "{e2e40e57-3df3-44e7-a308-6f7fee6ccc9e}", VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_AUTO);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::fixed_forward_list,  "{b06c5d90-e5f0-45d5-bbea-e83e3c6ff027}", VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_AUTO);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::array,               "{9fb62929-9ba8-4604-9305-c0a566501c52}", VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_AUTO);
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(VStd::bitset,              "{5b893452-8cc3-4543-94ac-b54d280938c7}", VOBJECT_INTERNAL_AUTO);

    // We do things a bit differently for fixed_string due to what appears to be a compiler bug in msbuild
    // Clang has no issue using the forward declaration directly
    template <typename Element, size_t MaxElementCount, typename Traits>
    using FixedStringTypeInfo = VStd::basic_fixed_string<Element, MaxElementCount, Traits>;
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_POSTFIX_ID(FixedStringTypeInfo, "{d3fe8d88-35cd-4d7d-8e8e-c274fe3a7941}", VOBJECT_INTERNAL_TYPENAME, VOBJECT_INTERNAL_AUTO, VOBJECT_INTERNAL_TYPENAME);

    static constexpr const char* _variantTypeId{ "{d6f289ed-1849-4f3f-b97c-dd34f2c40644}" };
    VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_PREFIX_ID(VStd::variant, "variant", _variantTypeId, VOBJECT_INTERNAL_TYPENAME_VARARGS);

    VOBJECT_INTERNAL_FUNCTION_VARIATION_SPECIALIZATION(VStd::function, "{3e106706-4b18-42c3-a488-be715b20fe25}");
} // namespace V

// Template class type info
#define VOBJECT_INTERNAL_TEMPLATE(_ClassName, _ClassUuid, ...)\
    void VOBJECT_Enable() {}\
    static const char* VOBJECT_Name() {\
         static char typeName[128] = { 0 };\
         if (typeName[0]==0) {\
            V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), #_ClassName "<");\
            V::Internal::AggregateTypes<__VA_ARGS__>::TypeName(typeName, V_ARRAY_SIZE(typeName));\
            V::Internal::VObjectSafeCat(typeName, V_ARRAY_SIZE(typeName), ">");\
                           }\
         return typeName; } \
    static const V::TypeId& VOBJECT_Id() {\
        static V::Internal::TypeIdHolder _uuid(V::TypeId(_ClassUuid) + V::Internal::AggregateTypes<__VA_ARGS__>::Id());\
        return _uuid;\
    }

// Template class type info
#define VOBJECT_INTERNAL_TEMPLATE_DEPRECATED(_ClassName, _ClassUuid, ...) \
    static_assert(false, "Embedded type info declaration inside templates are no longer supported. Please use 'VOBJECT_TEMPLATE' outside the template class instead.");

// all template versions are handled by a variadic template handler
#define VOBJECT_INTERNAL_3 VOBJECT_INTERNAL_TEMPLATE
#define VOBJECT_INTERNAL_4 VOBJECT_INTERNAL_TEMPLATE
#define VOBJECT_INTERNAL_5 VOBJECT_INTERNAL_TEMPLATE
#define VOBJECT_INTERNAL_6 VOBJECT_INTERNAL_TEMPLATE
#define VOBJECT_INTERNAL_7 VOBJECT_INTERNAL_TEMPLATE
#define VOBJECT_INTERNAL_8 VOBJECT_INTERNAL_TEMPLATE
#define VOBJECT_INTERNAL_9 VOBJECT_INTERNAL_TEMPLATE
#define VOBJECT_INTERNAL_10 VOBJECT_INTERNAL_TEMPLATE
#define VOBJECT_INTERNAL_11 VOBJECT_INTERNAL_TEMPLATE
#define VOBJECT_INTERNAL_12 VOBJECT_INTERNAL_TEMPLATE
#define VOBJECT_INTERNAL_13 VOBJECT_INTERNAL_TEMPLATE
#define VOBJECT_INTERNAL_14 VOBJECT_INTERNAL_TEMPLATE
#define VOBJECT_INTERNAL_15 VOBJECT_INTERNAL_TEMPLATE
#define VOBJECT_INTERNAL_16 VOBJECT_INTERNAL_TEMPLATE
#define VOBJECT_INTERNAL_17 VOBJECT_INTERNAL_TEMPLATE
#define VOBJECT_INTERNAL(...) V_MACRO_SPECIALIZE(VOBJECT_INTERNAL_, V_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))


#define VOBJECT_LEGACY VOBJECT_INTERNAL

#include <vcore/vobject/type_simple.h>

/**
* Use this macro outside a class to allow it to be identified across modules and serialized (in different contexts).
* The expected input is the class and the assigned uuid as a string or an instance of a uuid.
* Note that the VOBJECT_SPECIALIZE always has be declared in "namespace V".
* Example:
*   class MyClass
*   {
*   public:
*       ...
*   };
*
*   namespace V
*   {
*       VOBJECT_SPECIALIZE(MyClass, "{e193c899-15dc-4ccb-ba85-f671319f1740}");
*   }
*/
#define VOBJECT_SPECIALIZE(_ClassName, _ClassUuid) VOBJECT_INFO_INTERNAL_SPECIALIZE(_ClassName, _ClassUuid)

// Used to declare that a template argument is a "typename" with VOBJECT_TEMPLATE.
#define VOBJECT_TYPENAME VOBJECT_INTERNAL_TYPENAME
// Used to declare that a template argument is a "typename..." with VOBJECT_TEMPLATE.
#define VOBJECT_TYPENAME_VARARGS VOBJECT_INTERNAL_TYPENAME_VARARGS
// Used to declare that a template argument is a "class" with VOBJECT_TEMPLATE.
#define VOBJECT_CLASS VOBJECT_INTERNAL_CLASS
// Used to declare that a template argument is a "class..." with VOBJECT_TEMPLATE.
#define VOBJECT_CLASS_VARARGS VOBJECT_INTERNAL_CLASS_VARARGS
// Used to declare that a template argument is a number such as size_t with VOBJECT_TEMPLATE.
#define VOBJECT_AUTO VOBJECT_INTERNAL_AUTO

/**
* The same as VOBJECT_TEMPLATE, but allows the name to explicitly set. This was added for backwards compatibility.
* Prefer using VOBJECT_TEMPLATE and include the full namespace of the template class.
*/
#define VOBJECT_TEMPLATE_WITH_NAME(_Template, _Name, _Uuid, ...) VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_PREFIX_UUID(_Template, _Name, _Uuid, __VA_ARGS__)
/**
* Use this macro outside a template class to allow it to be identified across modules and serialized (in different contexts).
* The expected inputs are:
*   - The template
*   - The assigned uuid as a string or an instance of a uuid
*   - One or more template arguments.
* Note that the VOBJECT_TEMPLATE always has be declared in "namespace V".
* Example:
*   template<typename T1, size_t N, class T2>
*   class MyTemplateClass
*   {
*   public:
*       ...
*   };
*
*   namespace V
*   {
*       VOBJECT_TEMPLATE(MyTemplateClass, "{3e106706-4b18-42c3-a488-be715b20fe25}", VOBJECT_TYPENAME, VOBJECT_AUTO, VOBJECT_CLASS);
*   }
*/
#define VOBJECT_TEMPLATE(_Template, _Uuid, ...) VOBJECT_INTERNAL_SPECIALIZED_TEMPLATE_PREFIX_ID(_Template, #_Template, _Uuid, __VA_ARGS__)

#endif // V_FRAMEWORK_CORE_RTTI_TYPE_H