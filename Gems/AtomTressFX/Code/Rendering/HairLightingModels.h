/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Preprocessor/Enum.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            AZ_ENUM(HairLightingModel,
                GGX,
                Marschner,
                Kajiya);
        } // namespace Hair
    } // namespace Render
} // namespace AZ
