/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    namespace Render
    {
        namespace LightCulling
        {
            // These should match the numbers in LightCullingShared.azsli
            const uint32_t TileDimX = 16;
            const uint32_t TileDimY = 16;
            const uint32_t NumBinsPerTile = 32;
        }
    }
}
