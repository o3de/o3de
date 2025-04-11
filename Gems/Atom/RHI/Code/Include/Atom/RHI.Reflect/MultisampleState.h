/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/array.h>

#include <Atom/RHI.Reflect/Bits.h>
#include <Atom/RHI.Reflect/Limits.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    //! Defines a custom sample position when doing Multisample rendering.
    //! Sample positions have the origin(0, 0) at the pixel top left.
    //! Each of the X and Y coordinates are unsigned values in the range 0 (top / left) to Limits::Pipeline::MultiSampleCustomLocationGridSize - 1 (bottom / right).
    //! Values outside this range are invalid. Each increment of these integer values represents 1 / Limits::Pipeline::MultiSampleCustomLocationGridSize of a pixel.
    struct SamplePosition
    {
        AZ_TYPE_INFO(SamplePosition, "{8CCB872A-2CC3-4898-AB9E-C12517AC1FB8}");
        static void Reflect(ReflectContext* context);

        SamplePosition() = default;
        SamplePosition(uint8_t x, uint8_t y);

        bool operator==(const SamplePosition& other) const;
        bool operator!=(const SamplePosition& other) const;

        uint8_t m_x = 0;
        uint8_t m_y = 0;
    };

    struct MultisampleState
    {
        AZ_TYPE_INFO(MultisampleState, "{7673d8d8-c3db-462a-a155-cfe1d2331397}");
        static void Reflect(ReflectContext* context);

        MultisampleState() = default;
        MultisampleState(uint16_t samples, uint16_t quality);

        bool operator==(const MultisampleState& other) const;
        bool operator!=(const MultisampleState& other) const;

        AZStd::array<SamplePosition, Limits::Pipeline::MultiSampleCustomLocationsCountMax> m_customPositions{};
        uint32_t m_customPositionsCount = 0;
        uint16_t m_samples = 1;
        uint16_t m_quality = 0;
    };
}
