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
    void EditorTerrainPhysicsColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        // Call ReflectSubClass in EditorWrappedComponentBase to handle all the boilerplate reflection.
        BaseClassType::ReflectSubClass<EditorTerrainPhysicsColliderComponent, BaseClassType>(
            context, 1,
            &LmbrCentral::EditorWrappedComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType,
            typename BaseClassType::WrappedConfigType, 1>
        );
    }

    void EditorTerrainPhysicsColliderComponent::Activate()
    {
        UpdateConfigurationTagProvider();
        BaseClassType::Activate();
    }

    AZStd::unordered_set<AZ::u32> EditorTerrainPhysicsColliderComponent::GetSurfaceTagsInUse() const
    {
        AZStd::unordered_set<AZ::u32> tagsInUse;

        for (const TerrainPhysicsSurfaceMaterialMapping& mapping : m_configuration.m_surfaceMaterialMappings)
        {
            AZ::u32 crc = mapping.m_surfaceTag;
            tagsInUse.insert(crc);
        }

        return AZStd::move(tagsInUse);
    }

    AZ::u32 EditorTerrainPhysicsColliderComponent::ConfigurationChanged()
    {
        UpdateConfigurationTagProvider();
        return BaseClassType::ConfigurationChanged();
    }

    void EditorTerrainPhysicsColliderComponent::UpdateConfigurationTagProvider()
    {
        for (TerrainPhysicsSurfaceMaterialMapping& mapping : m_configuration.m_surfaceMaterialMappings)
        {
            mapping.SetTagListProvider(this);
        }
    }

}
