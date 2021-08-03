/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Module.h>

namespace Terrain
{
    class TerrainModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(TerrainModule, "{ccbd6b86-f43e-48ff-acb2-7eedaf495596}", AZ::Module);
        AZ_CLASS_ALLOCATOR(TerrainModule, AZ::SystemAllocator, 0);

        TerrainModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
