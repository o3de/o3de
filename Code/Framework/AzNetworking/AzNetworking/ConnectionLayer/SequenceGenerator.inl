/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
