/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeSafeIntegral.h>

namespace AzNetworking
{
    AZ_TYPE_SAFE_INTEGRAL(SequenceId, uint16_t);
    static constexpr SequenceId InvalidSequenceId = SequenceId{uint16_t(0xFFFF)};

    //! Helper method that compares wrap-around sequence values to determine if one is more recent than another.
    //! @param input1 first sequence value to compare against
    //! @param input2 second sequence value to compare against
    //! @return boolean true if input1 represents a newer sequence value than input2
    template <typename TYPE>
    bool SequenceMoreRecent(TYPE input1, TYPE input2);

    //! Helper method that returns true when the sequences appears to have wrapped around.
    //! @param input1 first sequence value to compare against
    //! @param input2 second sequence value to compare against
    //! @return boolean true if input1 represents a newer sequence value than input2 and input1 is numerically less than input2
    template <typename TYPE>
    bool SequenceRolledOver(TYPE input1, TYPE input2);

    //! @class SequenceGenerator
    //! @brief Generates wrapping sequence numbers.
    class SequenceGenerator
    {
    public:

        SequenceGenerator() = default;

        //! Resets the sequence generator instance.
        void Reset();

        //! Returns the next sequence id for this generator instance.
        //! @return the next sequence id for this generator instance
        SequenceId GetNextSequenceId();

    private:

        SequenceId m_nextSequenceId = InvalidSequenceId;
    };
}

AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(AzNetworking::SequenceId);

#include <AzNetworking/ConnectionLayer/SequenceGenerator.inl>
