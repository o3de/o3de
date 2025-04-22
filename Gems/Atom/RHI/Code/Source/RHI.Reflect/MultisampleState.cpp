/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/MultisampleState.h>

namespace AZ::RHI
{
    SamplePosition::SamplePosition(uint8_t x, uint8_t y)
        : m_x(x)
        , m_y(y)
    {
        AZ_Assert(
            m_x < Limits::Pipeline::MultiSampleCustomLocationGridSize && m_y < Limits::Pipeline::MultiSampleCustomLocationGridSize,
            "Invalid sample custom position (%d, %d)", m_x, m_y);
    }

    bool SamplePosition::operator != (const SamplePosition& other) const
    {
        return !(*this == other);
    }

    bool SamplePosition::operator == (const SamplePosition& other) const
    {
        return m_x == other.m_x && m_y == other.m_y;
    }

    MultisampleState::MultisampleState(uint16_t samples, uint16_t quality)
        : m_samples{samples}
        , m_quality{quality}
    {}

    bool MultisampleState::operator == (const MultisampleState& other) const
    {
        return m_customPositions == other.m_customPositions &&
                m_customPositionsCount == other.m_customPositionsCount &&
                m_samples == other.m_samples &&
                m_quality == other.m_quality;
    }

    bool MultisampleState::operator != (const MultisampleState& other) const
    {
        return !(*this == other);
    }
}
