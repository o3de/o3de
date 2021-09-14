/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorComponents/EditorTerrainPhysicsColliderComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace Terrain
{
    static constexpr const char* const CategoryName = "Terrain";
    static constexpr const char* const ComponentName = "Terrain Physics Collider";
    static constexpr const char* const ComponentDescription = "Provides terrain data to a physics collider in the form of a heightfield and surface->material mapping.";
    static constexpr const char* const Icon = "Editor/Icons/Components/TerrainLayerSpawner.svg";
    static constexpr const char* const ViewportIcon = "Editor/Icons/Components/Viewport/TerrainLayerSpawner.svg";
    static constexpr const char* const HelpUrl = "";

    void EditorTerrainPhysicsColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::ReflectSubClass<EditorTerrainPhysicsColliderComponent, BaseClassType>(
            context, 1,
            &LmbrCentral::EditorWrappedComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType,
            typename BaseClassType::WrappedConfigType, 1>
        );
    }
}
