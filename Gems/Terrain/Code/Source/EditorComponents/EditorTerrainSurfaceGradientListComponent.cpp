/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorComponents/EditorTerrainSurfaceGradientListComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace Terrain
{
    void EditorTerrainSurfaceGradientListComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::ReflectSubClass<EditorTerrainSurfaceGradientListComponent, BaseClassType>(
            context, 1,
            &LmbrCentral::EditorWrappedComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType,
            typename BaseClassType::WrappedConfigType, 1>
        );
    }

    void EditorTerrainSurfaceGradientListComponent::Activate()
    {
        UpdateConfigurationTagProvider();
        BaseClassType::Activate();
    }

    AZ::u32 EditorTerrainSurfaceGradientListComponent::ConfigurationChanged()
    {
        UpdateConfigurationTagProvider();
        return BaseClassType::ConfigurationChanged();
    }

    void EditorTerrainSurfaceGradientListComponent::UpdateConfigurationTagProvider()
    {
        for (TerrainSurfaceGradientMapping& mapping : m_configuration.m_gradientSurfaceMappings)
        {
            mapping.SetTagListProvider(this);
        }
    }

    AZStd::unordered_set<AZ::u32> EditorTerrainSurfaceGradientListComponent::GetSurfaceTagsInUse() const
    {
        AZStd::unordered_set<AZ::u32> tagsInUse;

        for (const TerrainSurfaceGradientMapping& mapping : m_configuration.m_gradientSurfaceMappings)
        {
            AZ::u32 crc = mapping.m_surfaceTag;
            tagsInUse.insert(crc);
        }

        return AZStd::move(tagsInUse);
    }
}
