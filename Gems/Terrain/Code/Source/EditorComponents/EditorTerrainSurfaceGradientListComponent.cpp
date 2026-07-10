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
#include <AzFramework/Translation/TranslationDef.h>

namespace Terrain
{
    void EditorTerrainSurfaceGradientListComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::ReflectSubClass<EditorTerrainSurfaceGradientListComponent, BaseClassType>(
            context, 1,
            &LmbrCentral::EditorWrappedComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType,
            typename BaseClassType::WrappedConfigType, 1>
        );

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            if (auto edit = serialize->GetEditContext())
            {
                edit->Class<TerrainSurfaceGradientMapping>(
                    QT_TRANSLATE_NOOP("Terrain", "Terrain Surface Gradient Mapping"),
                    QT_TRANSLATE_NOOP("Terrain", "Mapping between a gradient and a surface."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainSurfaceGradientMapping::m_gradientEntityId,
                        QT_TRANSLATE_NOOP("Terrain", "Gradient Entity"),
                        QT_TRANSLATE_NOOP("Terrain", "ID of Entity providing a gradient."))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->UIElement("GradientPreviewer", "Previewer")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ_CRC_CE("GradientEntity"), &TerrainSurfaceGradientMapping::m_gradientEntityId)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainSurfaceGradientMapping::m_surfaceTag,
                        QT_TRANSLATE_NOOP("Terrain", "Surface Tag"),
                        QT_TRANSLATE_NOOP("Terrain", "Surface type to map to this gradient."))
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &TerrainSurfaceGradientMapping::BuildSelectableTagList)
                    ;

                edit->Class<TerrainSurfaceGradientListConfig>(
                    QT_TRANSLATE_NOOP("Terrain", "Terrain Surface Gradient List Component"),
                    QT_TRANSLATE_NOOP("Terrain", "Provide mapping between gradients and surfaces."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainSurfaceGradientListConfig::m_gradientSurfaceMappings,
                        QT_TRANSLATE_NOOP("Terrain", "Gradient to Surface Mappings"),
                        QT_TRANSLATE_NOOP("Terrain", "Maps Gradient Entities to Surfaces."))
                    ;
            }
        }
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

    AZStd::unordered_set<SurfaceData::SurfaceTag> EditorTerrainSurfaceGradientListComponent::GetSurfaceTagsInUse() const
    {
        AZStd::unordered_set<SurfaceData::SurfaceTag> tagsInUse;

        for (const TerrainSurfaceGradientMapping& mapping : m_configuration.m_gradientSurfaceMappings)
        {
            tagsInUse.insert(mapping.m_surfaceTag);
        }

        return tagsInUse;
    }

    AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> TerrainSurfaceGradientMapping::BuildSelectableTagList() const
    {
        return Terrain::BuildSelectableTagList(m_tagListProvider, m_surfaceTag);
    }

    void TerrainSurfaceGradientMapping::SetTagListProvider(const EditorSurfaceTagListProvider* tagListProvider)
    {
        m_tagListProvider = tagListProvider;
    }
}
