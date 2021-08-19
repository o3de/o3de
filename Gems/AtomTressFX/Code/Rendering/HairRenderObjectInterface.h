/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Utils/StableDynamicArray.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            class HairRenderObjectInterface
            {
            public:
                AZ_RTTI(AZ::Render::HairRenderObjectInterface, "{21461ADB-A4F7-4021-BA2C-992B3E011D8E}");
            };

            using HairRenderObjectInterfaceHandle = StableDynamicArrayHandle<HairRenderObjectInterface>;
        } // namespace Hair
    } // namespace Render
} // namespace AZ
