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
    inline FixedSizeBitsetView::FixedSizeBitsetView(IBitset& bitset, uint32_t startOffset, uint32_t count)
        : m_bitset(bitset)
        , m_startOffset(startOffset)
        , m_count(startOffset < bitset.GetValidBitCount() && startOffset + count <= bitset.GetValidBitCount() ? count : 0)
    {
        AZ_Assert(startOffset + count <= bitset.GetValidBitCount(), "Out of bounds setup in BitsetSubset. Defaulting to 0 bit count.");
    }

    inline void FixedSizeBitsetView::SetBit(uint32_t index, bool value)
    {
        AZ_Assert(index < m_count, "Out of bounds access in BitsetSubset (requested %u, count %u)", index, m_count);
        if (m_count)
        {
            m_bitset.SetBit(m_startOffset + index, value);
        }
    }

    inline bool FixedSizeBitsetView::GetBit(uint32_t index) const
    {
        AZ_Assert(index < m_count, "Out of bounds access in BitsetSubset (requested %u, count %u)", index, m_count);
        if (m_count)
        {
            return m_bitset.GetBit(m_startOffset + index);
        }
        return false;
    }

    inline bool FixedSizeBitsetView::AnySet() const
    {
        for (uint32_t i = 0; i < m_count; ++i)
        {
            if (GetBit(i))
            {
                return true;
            }
        }

        return false;
    }

    inline uint32_t FixedSizeBitsetView::GetValidBitCount() const
    {
        return m_count;
    }
}
