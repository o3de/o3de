/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Uuid.h>

namespace AZ
{
    namespace Render
    {
        inline constexpr AZ::Uuid HDRiSkyboxComponentTypeId{ "{3FF41FC3-A18A-4C0A-94C4-FA353A6B661D}" };
        inline constexpr AZ::Uuid EditorHDRiSkyboxComponentTypeId{ "{B736789D-0101-4D17-A932-B5224EEFA8B4}" };

        inline constexpr AZ::Uuid PhysicalSkyComponentTypeId{ "{B5D15F12-359D-463D-8FCA-60DA97055812}" };
        inline constexpr AZ::Uuid EditorPhysicalSkyComponentTypeId{ "{A249BA4E-BBD5-45A1-86AA-FCA4B2C4CF9B}" };

        static const float PhysicalSkyDefaultIntensity = 4.0f; // default intensity for sky, in EV100 unit
        static const float PhysicalSunDefaultIntensity = 8.0f;  // default intensity for sun, in EV100 unit

        static const float PhysicalSunRadius = 0.695508f;                // 695,508 km
        static const float PhysicalSunDistance = 149.6f;                 // 149,600,000 km
        static const float PhysicalSunCosAngularDiameter = 0.999956835f; // radians
    } // namespace Render
} // namespace AZ
