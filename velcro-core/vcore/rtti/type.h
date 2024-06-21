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
#include <vcore/std/typetraits/void_t.h>
#include <vcore/std/function/invoke.h>
#include <vcore/math/uuid.h>

#include <cstdio> // for snprintf



namespace V {
    using TypeId = V::Uuid;

    /*
    inline namespace TypeIdResolverTags
    {

        struct PointerRemovedTypeIdTag
        {
        };

        struct CanonicalTypeIdTag
        {
        };

        struct GenericTypeIdTag
        {
        };
    }

    namespace Internal
    {
         V_HAS_MEMBER(VelcroTypeInfoIntrusive, TYPEINFO_Enable, void, ());

        template<typename T, typename = void>
        struct IsVelcroTypeInfoSpecialized : VStd::false_type {
        };

        template<typename T>
        struct IsVelcroTypeInfoSpecialized<T, VStd::enable_if_t<VStd::is_invocable_r<bool, decltype(&VelcroTypeInfo<T>::Specialized)>::value>>
            : VStd::true_type
        {
        };

        template <class T>
        struct VelcroTypeInfo
        {
            static constexpr bool value = IsVelcroTypeInfoSpecialized<T>::value || IsVelcroTypeInfoIntrusive<T>::value;
            using type = typename VStd::Utils::if_c<value, VStd::true_type, VStd::false_type>::type;
        };

        inline void VelcroTypeInfoSafeCat(char* destination, size_t maxDestination, const char* source)
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
                VelcroTypeInfoSafeCat(buffer, bufferSize, VelcroTypeInfo<T1>::Name());
                VelcroTypeInfoSafeCat(buffer, bufferSize, " ");
                AggregateTypes<Tn...>::TypeName(buffer, bufferSize);
            }

            template<typename TypeIdResolverTag = CanonicalTypeIdTag>
            static V::TypeId Uuid()
            {
                const V::TypeId tail = AggregateTypes<Tn...>::template Uuid<TypeIdResolverTag>();
                if (!tail.IsNull())
                {
                    // Avoid accumulating a null uuid, since it affects the result.
                    return VelcroTypeInfo<T1>::template Uuid<TypeIdResolverTag>() + tail;
                }
                else
                {
                    return VelcroTypeInfo<T1>::template Uuid<TypeIdResolverTag>();
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
    }

    namespace VelcroGenericTypeInfo
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
            return VelcroTypeInfo<T>::template Uuid<V::GenericTypeIdTag>();
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

    enum class TypeTraits : V::u64
    {
        //! Stores the result of the std::is_signed check for a registered type
        is_signed = 0b1,
        //! Stores the result of the std::is_unsigned check for a registered type
        //! NOTE: While this might seem to be redundant due to is_signed, it is NOT
        //! Only numeric types have a concept of signedness
        is_unsigned = 0b10,
        //! Stores the result of the std::is_enum check
        is_enum = 0b100
    };

    V_DEFINE_ENUM_BITWISE_OPERATORS(V::TypeTraits);

    template <class T, bool IsEnum = VStd::is_enum<T>::value>
    struct VelcroTypeInfo;

    template<class T>
    struct VelcroTypeInfo<T, false>
    {
        static const char* Name() {
            static_assert(V::Internal::IsVelcroTypeInfoIntrusive<T>::value, 
            "You should use VELCRO_TYPE_INFO or VELCRO_RTTI in your class/struct, or use VELCRO_TYPE_INFO_SPECIALIZE() externally." 
            "Make sure to include the header containing this information (usually your class header).");
            return T::TYPEINFO_Name();
        }

        template<typename TypeIdResolverTag = CanonicalTypeIdTag>
        static const V::TypeId& Uuid()
        {
            static_assert(V::Internal::IsVelcroTypeInfoIntrusive<T>::value,
                "You should use VELCRO_TYPE_INFO or VELCRO_RTTI in your class/struct, or use VELCRO_TYPE_INFO_SPECIALIZE() externally. "
                "Make sure to include the header containing this information (usually your class header).");
            return T::TYPEINFO_Uuid();
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
    }


      // Default Specialization for enums without an overload
    template <class T>
    struct VelcroTypeInfo<T, true>
    {
        typedef typename VStd::RemoveEnum<T>::type UnderlyingType;
        static const char* Name() { return nullptr; }
        template<typename TypeIdResolverTag = CanonicalTypeIdTag>
        static const V::TypeId& Uuid() { static V::TypeId nullUuid = V::TypeId::CreateNull(); return nullUuid; }
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
    inline bool operator==(VelcroTypeInfo<T> const& lhs, VelcroTypeInfo<U> const& rhs)
    {
        return lhs.Uuid() == rhs.Uuid();
    }

    template<class T, class U>
    inline bool operator!=(VelcroTypeInfo<T> const& lhs, VelcroTypeInfo<U> const& rhs)
    {
        return lhs.Uuid() != rhs.Uuid();
    }

    namespace Internal
    {
        // Sizeof helper to deal with incomplete types and the void type
        template<typename T, typename = void>
        struct TypeInfoSizeof
        {
            static constexpr size_t Size()
            {
                return 0;
            }
        };

        // In this case that sizeof(T) is a compilable, the type is complete
        template<typename T>
        struct TypeInfoSizeof<T, VStd::void_t<decltype(sizeof(T))>>
        {
            static constexpr size_t Size()
            {
                return sizeof(T);
            }
        };
    }

    // VelcroTypeInfo specialization helper for non intrusive TypeInfo
    #define VELCRO_TYPE_INFO_INTERNAL_SPECIALIZE(_ClassName, _ClassUuid)      \
            template<>                                                        \
            struct VelcroTypeInfo<_ClassName, VStd::is_enum<_ClassName>::value>  \
            {                                                                 \
                static const char* Name() { return #_ClassName; }             \
                template<typename TypeIdResolverTag = CanonicalTypeIdTag>     \
                static const V::TypeId& Uuid() {                              \
                    static V::Internal::TypeIdHolder _uuid(_ClassUuid);       \
                    return _uuid;                                             \
                }                                                             \
                static constexpr TypeTraits GetTypeTraits()                   \
                {                                                             \
                    TypeTraits typeTraits{};                                  \
                    typeTraits |= std::is_signed<VStd::RemoveEnumT<_ClassName>>::value ? TypeTraits::is_signed : typeTraits; \
                    typeTraits |= std::is_unsigned<VStd::RemoveEnumT<_ClassName>>::value ? TypeTraits::is_unsigned : typeTraits; \
                    typeTraits |= std::is_enum<_ClassName>::value ? TypeTraits::is_enum: typeTraits; \
                    return typeTraits;                                        \
                }                                                             \
                static constexpr size_t Size()                                \
                {                                                             \
                    return V::Internal::TypeInfoSizeof<_ClassName>::Size();   \
                }                                                             \
                static bool Specialized() { return true; }                    \
            };

    // specialize 函数指针
    template<class R, class... Args>
    struct VelcroTypeInfo<R(Args...), false>
    {
        static const char* Name()
        {
            static char typeName[64] = { 0 };
            if (typeName[0] == 0)
            {
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), "{");
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), V::VelcroTypeInfo<R>::Name());
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), "(");
                V::Internal::AggregateTypes<Args...>::TypeName(typeName, V_ARRAY_SIZE(typeName));
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), ")}");
            }
            return typeName;
        }
        template<typename TypeIdResolverTag = CanonicalTypeIdTag>
        static const V::TypeId& Uuid()
        {
            static V::Internal::TypeIdHolder _uuid(V::Internal::AggregateTypes<R, Args...>::template Uuid<TypeIdResolverTag>());
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
    struct VelcroTypeInfo<R(C::*)(Args...), false>
    {
        static const char* Name()
        {
            static char typeName[128] = { 0 };
            if (typeName[0] == 0)
            {
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), "{");
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), V::VelcroTypeInfo<R>::Name());
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), "(");
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), V::VelcroTypeInfo<C>::Name());
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), "::*)");
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), "(");
                V::Internal::AggregateTypes<Args...>::TypeName(typeName, V_ARRAY_SIZE(typeName));
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), ")}");
            }
            return typeName;
        }
        template<typename TypeIdResolverTag = CanonicalTypeIdTag>
        static const V::TypeId& Uuid()
        {
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
    struct VelcroTypeInfo<R(C::*)(Args...) const, false>
        : VelcroTypeInfo<R(C::*)(Args...), false> {};

    // specialize for member data pointers
    template<class R, class C>
    struct VelcroTypeInfo<R C::*, false>
    {
        static const char* Name()
        {
            static char typeName[64] = { 0 };
            if (typeName[0] == 0)
            {
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), "{");
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), V::VelcroTypeInfo<R>::Name());
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), " ");
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), V::VelcroTypeInfo<C>::Name());
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), "::*}");
            }
            return typeName;
        }
        template<typename TypeIdResolverTag = CanonicalTypeIdTag>
        static const V::TypeId& Uuid()
        {
            static V::Internal::TypeIdHolder s_uuid(V::Internal::AggregateTypes<R, C>::template Uuid<TypeIdResolverTag>());
            return s_uuid;
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

// Helper macro to generically specialize const, references, pointers, etc
#define VELCRO_TYPE_INFO_INTERNAL_SPECIALIZE_CV(_T1, _Specialization, _NamePrefix, _NameSuffix)                       \
    template<class _T1>                                                                                               \
    struct VelcroTypeInfo<_Specialization, false> {                                                                   \
        static const char* Name() {                                                                                   \
            static char typeName[64] = { 0 };                                                                         \
            if (typeName[0] == 0) {                                                                                   \
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), _NamePrefix);                    \
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), V::VelcroTypeInfo<_T1>::Name()); \
                V::Internal::VelcroTypeInfoSafeCat(typeName, V_ARRAY_SIZE(typeName), _NameSuffix);                    \
            }                                                                                                         \
            return typeName;                                                                                          \
        }                                                                                                             \
        template<typename TypeIdResolverTag = PointerRemovedTypeIdTag>                                                \
        static const V::TypeId& Uuid() {                                                                              \
            static V::Internal::TypeIdHolder _uuid(                                                                   \
                !VStd::is_same<TypeIdResolverTag, PointerRemovedTypeIdTag>::value                                     \
                && VStd::is_pointer<_Specialization>::value ?                                                         \
                V::VelcroTypeInfo<_T1>::template Uuid<TypeIdResolverTag>() + V::Internal::PointerTypeId() : V::VelcroTypeInfo<_T1>::template Uuid<TypeIdResolverTag>()); \
            return _uuid;                                                                                             \
        }                                                                                                             \
        static constexpr TypeTraits GetTypeTraits()                                                                   \
        {                                                                                                             \
            TypeTraits typeTraits{};                                                                                  \
            typeTraits |= std::is_signed<_Specialization>::value ? TypeTraits::is_signed : typeTraits;                \
            typeTraits |= std::is_unsigned<_Specialization>::value ? TypeTraits::is_unsigned : typeTraits;            \
            return typeTraits;                                                                                        \
        }                                                                                                             \
        static constexpr size_t Size()                                                                                \
        {                                                                                                             \
            return sizeof(_Specialization);                                                                           \
        }                                                                                                             \
        static bool Specialized() { return true; }                                                                    \
    }

// Helper macros to generically specialize template types
#define  VELCRO_TYPE_INFO_INTERNAL_VARIATION_GENERIC(_Generic, _Uuid)                                                 \
    namespace VelcroGenericTypeInfo {                                                                                 \
        template<>                                                                                                    \
        inline const V::TypeId& Uuid<_Generic>(){ static V::Internal::TypeIdHolder _uuid(_Uuid); return _uuid; }      \
    }*/
}

#define VELCRO_TYPE_INFO(_ClassName, _ClassUuid)                                             \
    void TYPEINFO_Enable(){}                                                                 \
    static const char* TYPEINFO_Name() { return #_ClassName; }                               \
    static const V::TypeId& TYPEINFO_Uuid() { static V::TypeId _uuid(_ClassUuid); return _uuid; }

#endif // V_FRAMEWORK_CORE_RTTI_TYPE_H