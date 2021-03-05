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

namespace AZ
{
    namespace Render
    {
        static constexpr const char* const HDRiSkyboxComponentTypeId = "{3FF41FC3-A18A-4C0A-94C4-FA353A6B661D}";
        static constexpr const char* const EditorHDRiSkyboxComponentTypeId = "{B736789D-0101-4D17-A932-B5224EEFA8B4}";

        static constexpr const char* const PhysicalSkyComponentTypeId = "{B5D15F12-359D-463D-8FCA-60DA97055812}";
        static constexpr const char* const EditorPhysicalSkyComponentTypeId = "{A249BA4E-BBD5-45A1-86AA-FCA4B2C4CF9B}";

        static const float PhysicalSkyDefaultIntensity = 4.0f; // default intensity for sky, in EV100 unit
        static const float PhysicalSunDefaultIntensity = 8.0f;  // default intensity for sun, in EV100 unit

        static const float PhysicalSunRadius = 0.695508f;                // 695,508 km
        static const float PhysicalSunDistance = 149.6f;                 // 149,600,000 km
        static const float PhysicalSunCosAngularDiameter = 0.999956835f; // radians
    } // namespace Render
} // namespace AZ
