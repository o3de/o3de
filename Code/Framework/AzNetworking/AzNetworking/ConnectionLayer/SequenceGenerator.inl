/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    template <typename TYPE>
    inline bool SequenceMoreRecent(TYPE input1, TYPE input2)
    {
        constexpr TYPE HalfMaxSequence = static_cast<TYPE>(static_cast<TYPE>(~0) >> 1);
        return ((input1 > input2) && (input1 - input2 <= HalfMaxSequence)) ||
               ((input2 > input1) && (input2 - input1 >  HalfMaxSequence));
    }

    template <typename TYPE>
    inline bool SequenceRolledOver(TYPE input1, TYPE input2)
    {
        constexpr TYPE HalfMaxSequence = static_cast<TYPE>(static_cast<TYPE>(~0) >> 1);
        return (input2 > input1) && (input2 - input1 > HalfMaxSequence);
    }

    inline void SequenceGenerator::Reset()
    {
        m_nextSequenceId = InvalidSequenceId;
    }

    inline SequenceId SequenceGenerator::GetNextSequenceId()
    {
        ++m_nextSequenceId;

        if (m_nextSequenceId == InvalidSequenceId)
        {
            ++m_nextSequenceId;
        }

        return m_nextSequenceId;
    }
}
