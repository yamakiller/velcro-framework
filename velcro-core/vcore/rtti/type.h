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



namespace V {
    using TypeId = V::Uuid;
    struct Adl {};

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

    namespace Internal {
        V_HAS_MEMBER(VObjectIntrusive, VOBJECT_Enable, void, ());

        template<class T, bool IsEnum = VStd::is_enum<T>::value>
        struct VObject;

        

        namespace AzGenericTypeInfo
        {
            /// Needs to match declared parameter type.
            template <template <typename...> class> constexpr bool false_v1 = false;
            template <template <VStd::size_t...> class> constexpr bool false_v2 = false;
            template <template <typename, VStd::size_t> class> constexpr bool false_v3 = false;
            template <template <typename, typename, VStd::size_t> class> constexpr bool false_v4 = false;
            template <template <typename, typename, typename, VStd::size_t> class> constexpr bool false_v5 = false;
            template <template <typename, VStd::size_t, typename> class> constexpr bool false_v6 = false;

            template<typename T>
            inline const V::TypeId& Uuid()
            {
                return VObject<T>::template Uuid<V::GenericTypeIdTag>();
            }

            template<template<typename...> class T>
            inline const V::TypeId& Uuid()
            {
                static_assert(false_v1<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
                static const V::TypeId _uuid = V::TypeId::CreateNull();
                return _uuid;
            }

            template<template<VStd::size_t...> class T>
            inline const V::TypeId& Uuid()
            {
                static_assert(false_v2<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
                static const V::TypeId _uuid = V::TypeId::CreateNull();
                return _uuid;
            }

            template<template<typename, VStd::size_t> class T>
            inline const V::TypeId& Uuid()
            {
                static_assert(false_v3<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
                static const V::TypeId _uuid = V::TypeId::CreateNull();
                return _uuid;
            }

            template<template<typename, typename, VStd::size_t> class T>
            inline const V::TypeId& Uuid()
            {
                static_assert(false_v4<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
                static const V::TypeId _uuid = V::TypeId::CreateNull();
                return _uuid;
            }

            template<template<typename, typename, typename, VStd::size_t> class T>
            inline const V::TypeId& Uuid()
            {
                static_assert(false_v5<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
                static const V::TypeId _uuid = V::TypeId::CreateNull();
                return _uuid;
            }

            template<template<typename, VStd::size_t, typename> class T>
            inline const V::TypeId& Uuid()
            {
                static_assert(false_v6<T>, "Missing specialization for this template. Make sure it's registered for type info support.");
                static const V::TypeId _uuid = V::TypeId::CreateNull();
                return _uuid;
            }
        }

        template <class T>
        struct HasVObject
        {
            static constexpr bool value =  HasVObjectIntrusive<T>::value;
            using type = typename VStd::Utils::if_c<value, VStd::true_type, VStd::false_type>::type;
        };

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
            static V::TypeId Uuid()
            {
                const V::TypeId tail = AggregateTypes<Tn...>::template Uuid<TypeIdResolverTag>();
                if (!tail.IsNull())
                {
                    // Avoid accumulating a null uuid, since it affects the result.
                    return VObject<T1>::template Uuid<TypeIdResolverTag>() + tail;
                }
                else
                {
                    return VObject<T1>::template Uuid<TypeIdResolverTag>();
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
            static V::TypeId Uuid()
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
    }

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

    template <class T, bool IsEnum = VStd::is_enum<T>::value>
    struct VObject;

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
        static const V::TypeId& Uuid()
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

    #define VOBJECT_INTERNAL_SPECIALIZE_CV(_T1, _Specialization, _NamePrefix, _NameSuffix)                      \
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
        static const V::TypeId& Uuid() {                                                                         \
            static V::Internal::TypeIdHolder _uuid(                                                              \
                !VStd::is_same<TypeIdResolverTag, PointerRemovedTypeIdTag>::value                                \
                && VStd::is_pointer<_Specialization>::value ?                                                    \
                V::VObject<_T1>::template Uuid<TypeIdResolverTag>() + V::Internal::PointerTypeId() : V::VObject<_T1>::template Uuid<TypeIdResolverTag>()); \
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
} // namespace V 


#define VOBJECT(_ClassName, _ClassUuid)                                                               \
    void  VOBJECT_Enable() {}                                                                         \
    static const char* VOBJECT_Name() { return #_ClassName; }                                         \
    static const V::TypeId& VOBJECT_Id() { static V::TypeId _uuid(_ClassUuid); return _uuid; }        \
    VOBJECT_INFO_INTERNAL_SPECIALIZE(_ClassName, _ClassUuid) 



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

}



#endif // V_FRAMEWORK_CORE_RTTI_TYPE_H