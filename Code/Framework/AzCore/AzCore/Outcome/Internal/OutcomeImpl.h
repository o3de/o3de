/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ
{
    //////////////////////////////////////////////////////////////////////////
    // Success Implementation

    constexpr AZStd::in_place_t Success()
    {
        return AZStd::in_place;
    }

    template <class ValueT, class>
    constexpr ValueT Success(ValueT&& rhs)
    {
        return AZStd::forward<ValueT>(rhs);
    }

    //////////////////////////////////////////////////////////////////////////
    // Failure Implementation

    constexpr AZStd::unexpected<AZStd::unexpect_t> Failure()
    {
        return AZStd::unexpected{ AZStd::unexpect };
    }

    template <class ValueT, class>
    constexpr AZStd::unexpected<ValueT> Failure(ValueT&& rhs)
    {
        return AZStd::unexpected<ValueT>(AZStd::forward<ValueT>(rhs));
    }

    ////////////////////////////////////////////////////////////////////////////
    // Outcome Implementation
    template <class ValueT, class ErrorT>
    AZ_FORCE_INLINE Outcome<ValueT, ErrorT>::Outcome()
        : BaseType{ AZStd::unexpect }
    {
    }

    template <class ValueT, class ErrorT>
    AZ_FORCE_INLINE Outcome<ValueT, ErrorT>::Outcome(const SuccessType& success)
        : BaseType{ success }
    {
    }

    template <class ValueT, class ErrorT>
    AZ_FORCE_INLINE Outcome<ValueT, ErrorT>::Outcome(SuccessType&& success)
        : BaseType{ AZStd::move(success) }
    {
    }

    template <class ValueT, class ErrorT>
    AZ_FORCE_INLINE Outcome<ValueT, ErrorT>::Outcome(const FailureType& failure)
        : BaseType{ failure }
    {
    }

    template <class ValueT, class ErrorT>
    AZ_FORCE_INLINE Outcome<ValueT, ErrorT>::Outcome(FailureType&& failure)
        : BaseType{ AZStd::move(failure) }
    {
    }

    template <class ValueT, class ErrorT>
    AZ_FORCE_INLINE Outcome<ValueT, ErrorT>::Outcome(const Outcome& other) = default;

    template <class ValueT, class ErrorT>
    AZ_FORCE_INLINE Outcome<ValueT, ErrorT>::Outcome(Outcome&& other) = default;

    template <class ValueT, class ErrorT>
    AZ_FORCE_INLINE Outcome<ValueT, ErrorT>::~Outcome() = default;

    template <class ValueT, class ErrorT>
    AZ_FORCE_INLINE Outcome<ValueT, ErrorT>& Outcome<ValueT, ErrorT>::operator=(const Outcome& other) = default;

    template <class ValueT, class ErrorT>
    AZ_FORCE_INLINE Outcome<ValueT, ErrorT>& Outcome<ValueT, ErrorT>::operator=(Outcome&& other) = default;

    template <class ValueT, class ErrorT>
    AZ_FORCE_INLINE Outcome<ValueT, ErrorT>& Outcome<ValueT, ErrorT>::operator=(const SuccessType& success)
    {
        static_cast<BaseType&>(*this) = success;
        return *this;
    }

    template <class ValueT, class ErrorT>
    AZ_FORCE_INLINE Outcome<ValueT, ErrorT>& Outcome<ValueT, ErrorT>::operator=(const FailureType& failure)
    {
        static_cast<BaseType&>(*this) = failure;
        return *this;
    }


    template <class ValueT, class ErrorT>
    AZ_FORCE_INLINE Outcome<ValueT, ErrorT>& Outcome<ValueT, ErrorT>::operator=(SuccessType&& success)
    {
        static_cast<BaseType&>(*this) = AZStd::move(success);
        return *this;
    }

    template <class ValueT, class ErrorT>
    AZ_FORCE_INLINE Outcome<ValueT, ErrorT>& Outcome<ValueT, ErrorT>::operator=(FailureType&& failure)
    {
        static_cast<BaseType&>(*this) = AZStd::move(failure);
        return *this;
    }

    template <class ValueT, class ErrorT>
    AZ_FORCE_INLINE bool Outcome<ValueT, ErrorT>::IsSuccess() const
    {
        return this->has_value();
    }

    template <class ValueT, class ErrorT>
    AZ_FORCE_INLINE Outcome<ValueT, ErrorT>::operator bool() const
    {
        return this->has_value();
    }

    template <class ValueT, class ErrorT>
    template <class Value_Type, class>
    AZ_FORCE_INLINE Value_Type& Outcome<ValueT, ErrorT>::GetValue()
    {
        return this->value();
    }

    template <class ValueT, class ErrorT>
    template <class Value_Type, class>
    AZ_FORCE_INLINE const Value_Type& Outcome<ValueT, ErrorT>::GetValue() const
    {
        return this->value();
    }

    template <class ValueT, class ErrorT>
    template <class Value_Type, class>
    AZ_FORCE_INLINE Value_Type && Outcome<ValueT, ErrorT>::TakeValue()
    {
        return AZStd::move(this->value());
    }

    template <class ValueT, class ErrorT>
    template <class U, class Value_Type, class>
    AZ_FORCE_INLINE Value_Type Outcome<ValueT, ErrorT>::GetValueOr(U&& defaultValue) const
    {
        return this->value_or(AZStd::forward<U>(defaultValue));
    }

    template <class ValueT, class ErrorT>
    template <class Error_Type, class>
    AZ_FORCE_INLINE Error_Type& Outcome<ValueT, ErrorT>::GetError()
    {
        return this->error();
    }

    template <class ValueT, class ErrorT>
    template <class Error_Type, class>
    AZ_FORCE_INLINE const Error_Type& Outcome<ValueT, ErrorT>::GetError() const
    {
        return this->error();
    }

    template <class ValueT, class ErrorT>
    template <class Error_Type, class>
    AZ_FORCE_INLINE Error_Type&& Outcome<ValueT, ErrorT>::TakeError()
    {
        return AZStd::move(this->error());
    }
} // namespace AZ
