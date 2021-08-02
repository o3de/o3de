/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "AzCore/std/typetraits/conditional.h"
#include "AzCore/std/typetraits/is_arithmetic.h"
#include "AzCore/std/typetraits/is_enum.h"

/*
// Lossy casts are just a wrapper around static_cast, but indicate the *intent* that numeric data loss
// has been accounted for. This is only meant for lossy numeric casting, so expect compile errors if
// used with other types.
*/
template <typename ToType, typename FromType>
inline constexpr AZStd::enable_if_t<
    (AZStd::is_arithmetic<FromType>::value || AZStd::is_enum<FromType>::value)
    && (AZStd::is_arithmetic<ToType>::value || AZStd::is_enum<ToType>::value)
    , ToType > azlossy_cast(FromType value)
{
    return static_cast<ToType>(value);
}

// This is a helper class that lets us induce the destination type of a lossy numeric cast.
// It should never be directly used by anything other than azlossy_caster.
namespace AZ
{
    template <typename FromType>
    class LossyCasted
    {
    public:
        explicit constexpr LossyCasted(FromType value)
            : m_value(value) { }
        template <typename ToType>
        constexpr operator ToType() const { return azlossy_cast<ToType>(m_value); }

    private:
        LossyCasted() = delete;
        void operator=(LossyCasted const&) = delete;

        FromType m_value;
    };
}

// This is the primary function we should use when lossy casting, since it induces the type we need
// to cast to from the code rather than requiring an explicit coupling in the source.
template <typename FromType>
inline constexpr AZ::LossyCasted<FromType> azlossy_caster(FromType value)
{
    return AZ::LossyCasted<FromType>(value);
}
