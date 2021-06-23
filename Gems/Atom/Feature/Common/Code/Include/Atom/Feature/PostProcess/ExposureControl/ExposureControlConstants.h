/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>

namespace AZ
{
    namespace Render
    {
        namespace ExposureControl
        {
            // Exposure Control Constants...
            enum class ExposureControlType
            {
                ManualOnly = 0,
                EyeAdaptation,

                ExposureControlTypeMax
            };
        }
    }
}
