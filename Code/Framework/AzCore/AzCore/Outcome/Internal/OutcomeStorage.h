/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AzCore/std/typetraits/aligned_storage.h"
#include "AzCore/std/typetraits/alignment_of.h"
#include "AzCore/std/typetraits/is_object.h"
#include "AzCore/std/typetraits/is_void.h"
#include "AzCore/std/typetraits/decay.h"
#include "AzCore/std/utils.h"

namespace AZ
{
    namespace Internal
    {
        // Shorthand for:
        // if Type != void:
        //   Type
        // else:
        //   Don't instantiate
        template <class Type>
        using enable_if_not_void = typename AZStd::enable_if<AZStd::is_void<Type>::value == false, Type>::type;

        // Shorthand for:
        // if is_valid(new Type(ArgsT...)):
        //   Type
        // else:
        //   Don't instantiate
        template <class Type, class ... ArgsT>
        using enable_if_constructible = typename AZStd::enable_if<AZStd::is_constructible<Type, ArgsT...>::value, Type>::type;

        //! Storage for Outcome's Success and Failure types.
        //! A specialization of the OutcomeStorage class handles void types.
        template <class ValueT, AZ::u8 instantiationNum>
        struct OutcomeStorage
        {
            using ValueType = ValueT;

            static_assert(AZStd::is_object<ValueType>::value,
                "Cannot instantiate Outcome using non-object type (except for void).");

            OutcomeStorage() = delete;

            OutcomeStorage(const ValueType& value)
                : m_value(value)
            {}

            OutcomeStorage(ValueType&& value)
                : m_value(AZStd::move(value))
            {}

            OutcomeStorage(const OutcomeStorage& other)
                : m_value(other.m_value)
            {}

            OutcomeStorage(OutcomeStorage&& other)
                : m_value(AZStd::move(other.m_value))
            {}

            OutcomeStorage& operator=(const OutcomeStorage& other)
            {
                m_value = other.m_value;
                return *this;
            }

            OutcomeStorage& operator=(OutcomeStorage&& other)
            {
                m_value = AZStd::move(other.m_value);
                return *this;
            }

            ValueType m_value;
        };

        //! Specialization of OutcomeStorage for void types.
        template <AZ::u8 instantiationNum>
        struct OutcomeStorage<void, instantiationNum>
        {
        };
    } // namespace Internal


    //////////////////////////////////////////////////////////////////////////
    // Aliases

    //! Aliases for usage with AZ::Success() and AZ::Failure()
    template <class ValueT>
    using SuccessValue = Internal::OutcomeStorage<typename AZStd::decay<ValueT>::type, 0u>;
    template <class ValueT>
    using FailureValue = Internal::OutcomeStorage<typename AZStd::decay<ValueT>::type, 1u>;
} // namespace AZ
