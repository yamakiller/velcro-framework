#ifndef V_FRAMEWORK_CORE_OUTCOMME_INTERNAL_OUTCOMME_STORAGE_H
#define V_FRAMEWORK_CORE_OUTCOMME_INTERNAL_OUTCOMME_STORAGE_H

#include "vcore/std/typetraits/aligned_storage.h"
#include "vcore/std/typetraits/alignment_of.h"
#include "vcore/std/typetraits/is_object.h"
#include "vcore/std/typetraits/is_void.h"
#include "vcore/std/typetraits/decay.h"
#include "vcore/std/utils.h"

namespace V
{
    namespace Internal
    {
        // Shorthand for:
        // if Type != void:
        //   Type
        // else:
        //   Don't instantiate
        template <class Type>
        using enable_if_not_void = typename VStd::enable_if<VStd::is_void<Type>::value == false, Type>::type;

        // Shorthand for:
        // if is_valid(new Type(ArgsT...)):
        //   Type
        // else:
        //   Don't instantiate
        template <class Type, class ... ArgsT>
        using enable_if_constructible = typename VStd::enable_if<VStd::is_constructible<Type, ArgsT...>::value, Type>::type;

        //! Storage for Outcome's Success and Failure types.
        //! A specialization of the OutcomeStorage class handles void types.
        template <class ValueT, V::u8 instantiationNum>
        struct OutcomeStorage
        {
            using ValueType = ValueT;

            static_assert(VStd::is_object<ValueType>::value,
                "Cannot instantiate Outcome using non-object type (except for void).");

            OutcomeStorage() = delete;

            OutcomeStorage(const ValueType& value)
                : m_value(value)
            {}

            OutcomeStorage(ValueType&& value)
                : m_value(VStd::move(value))
            {}

            OutcomeStorage(const OutcomeStorage& other)
                : m_value(other.m_value)
            {}

            OutcomeStorage(OutcomeStorage&& other)
                : m_value(VStd::move(other.m_value))
            {}

            OutcomeStorage& operator=(const OutcomeStorage& other)
            {
                m_value = other.m_value;
                return *this;
            }

            OutcomeStorage& operator=(OutcomeStorage&& other)
            {
                m_value = VStd::move(other.m_value);
                return *this;
            }

            ValueType m_value;
        };

        //! Specialization of OutcomeStorage for void types.
        template <V::u8 instantiationNum>
        struct OutcomeStorage<void, instantiationNum>
        {
        };
    } // namespace Internal


    //////////////////////////////////////////////////////////////////////////
    // Aliases

    //! Aliases for usage with V::Success() and V::Failure()
    template <class ValueT>
    using SuccessValue = Internal::OutcomeStorage<typename VStd::decay<ValueT>::type, 0u>;
    template <class ValueT>
    using FailureValue = Internal::OutcomeStorage<typename VStd::decay<ValueT>::type, 1u>;
} // namespace V

#endif // V_FRAMEWORK_CORE_OUTCOMME_INTERNAL_OUTCOMME_STORAGE_H