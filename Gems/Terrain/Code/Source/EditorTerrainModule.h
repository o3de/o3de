/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TerrainModule.h>

namespace Terrain
{
    class EditorTerrainModule
        : public TerrainModule
    {
    public:
        AZ_RTTI(EditorTerrainModule, "{68693F28-7051-4C14-85EA-DE6FD8CFCBD6}", TerrainModule);
        AZ_CLASS_ALLOCATOR(EditorTerrainModule, AZ::SystemAllocator);

        EditorTerrainModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
