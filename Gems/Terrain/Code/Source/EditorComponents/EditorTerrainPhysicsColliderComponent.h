/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/TerrainPhysicsColliderComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace Terrain
{
    class EditorTerrainPhysicsColliderComponent
        : public LmbrCentral::EditorWrappedComponentBase<TerrainPhysicsColliderComponent, TerrainPhysicsColliderConfig>
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<TerrainPhysicsColliderComponent, TerrainPhysicsColliderConfig>;
        AZ_EDITOR_COMPONENT(EditorTerrainPhysicsColliderComponent, "{C43FAB8F-3968-46A6-920E-E84AEDED3DF5}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);
    };
}
