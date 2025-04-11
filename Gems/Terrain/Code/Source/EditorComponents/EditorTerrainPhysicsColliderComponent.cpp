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
#include <AzFramework/Physics/Material/PhysicsMaterialManager.h>

namespace Terrain
{
    static AZ::Data::AssetId GetDefaultPhysicsMaterialAssetId()
    {
        // Used for Edit Context.
        // When the physics material asset property doesn't have an asset assigned it
        // will show "(default)" to indicate that the default material will be used.
        if (auto* materialManager = AZ::Interface<Physics::MaterialManager>::Get())
        {
            if (AZStd::shared_ptr<Physics::Material> defaultMaterial = materialManager->GetDefaultMaterial())
            {
                return defaultMaterial->GetMaterialAsset().GetId();
            }
        }
        return {};
    }

    void EditorTerrainPhysicsColliderComponent::Reflect(AZ::ReflectContext* context)
    {

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorTerrainPhysicsColliderComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("Configuration", &EditorTerrainPhysicsColliderComponent::m_configuration)
                ;

            if (auto edit = serialize->GetEditContext())
            {
                edit->Class<TerrainPhysicsSurfaceMaterialMapping>(
                    "Terrain Surface Material Mapping", "Mapping between a surface and a physics material.")

                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)

                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &TerrainPhysicsSurfaceMaterialMapping::m_surfaceTag, "Surface Tag",
                        "Surface type to map to a physics material.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &TerrainPhysicsSurfaceMaterialMapping::BuildSelectableTagList)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainPhysicsSurfaceMaterialMapping::m_materialAsset, "Material Asset", "")
                    ->Attribute(AZ::Edit::Attributes::DefaultAsset, &GetDefaultPhysicsMaterialAssetId)
                    ->Attribute(AZ_CRC_CE("EditButton"), "")
                    ->Attribute(AZ_CRC_CE("EditDescription"), "Open in Asset Editor")
                    ->Attribute(AZ_CRC_CE("DisableEditButtonWhenNoAssetSelected"), true)
                    ;

                edit->Class<TerrainPhysicsColliderConfig>(
                    "Terrain Physics Collider Component",
                    "Provides terrain data to a physics collider with configurable surface mappings.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainPhysicsColliderConfig::m_defaultMaterialAsset,
                        "Default Surface Physics Material", "Select a material to be used by unmapped surfaces by default")
                        ->Attribute(AZ::Edit::Attributes::DefaultAsset, &GetDefaultPhysicsMaterialAssetId)
                        ->Attribute(AZ_CRC_CE("EditButton"), "")
                        ->Attribute(AZ_CRC_CE("EditDescription"), "Open in Asset Editor")
                        ->Attribute(AZ_CRC_CE("DisableEditButtonWhenNoAssetSelected"), true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainPhysicsColliderConfig::m_surfaceMaterialMappings,
                        "Surface to Material Mappings", "Maps surfaces to physics materials")
                    ;

                edit->Class<EditorTerrainPhysicsColliderComponent>(
                    EditorTerrainPhysicsColliderComponent::s_componentName,
                    EditorTerrainPhysicsColliderComponent::s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, EditorTerrainPhysicsColliderComponent::s_icon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.o3de.org/docs/user-guide/components/reference/terrain/terrain-physics-collider/")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, EditorTerrainPhysicsColliderComponent::s_viewportIcon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, EditorTerrainPhysicsColliderComponent::s_helpUrl)
                    ->Attribute(AZ::Edit::Attributes::Category, EditorTerrainPhysicsColliderComponent::s_categoryName)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorTerrainPhysicsColliderComponent::m_configuration, "Config", "Terrain Physics Collider configuration")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorTerrainPhysicsColliderComponent::ConfigurationChanged);
                    ;
            }
        }
    }

    void EditorTerrainPhysicsColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
    }

    void EditorTerrainPhysicsColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
    }

    void EditorTerrainPhysicsColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
    }

    void EditorTerrainPhysicsColliderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        // If any of the following appear on the same entity as this one, they should get activated first as their data will
        // affect this component.
        services.push_back(AZ_CRC_CE("TerrainAreaService"));
        services.push_back(AZ_CRC_CE("TerrainHeightProviderService"));
        services.push_back(AZ_CRC_CE("TerrainSurfaceProviderService"));
    }

    void EditorTerrainPhysicsColliderComponent::Init()
    {
        m_component.Init();
    }

    void EditorTerrainPhysicsColliderComponent::Activate()
    {
        UpdateConfigurationTagProvider();
        m_component.SetEntity(GetEntity());
        m_component.UpdateConfiguration(m_configuration);
        m_component.Activate();
        AzToolsFramework::Components::EditorComponentBase::Activate();
    }

    void EditorTerrainPhysicsColliderComponent::Deactivate()
    {
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
        m_component.Deactivate();
        // remove the entity association, in case the parent component is being removed, otherwise the component will be reactivated
        m_component.SetEntity(nullptr);
    }

    void EditorTerrainPhysicsColliderComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->AddComponent(aznew TerrainPhysicsColliderComponent(m_configuration));
    }

    AZStd::unordered_set<SurfaceData::SurfaceTag> EditorTerrainPhysicsColliderComponent::GetSurfaceTagsInUse() const
    {
        AZStd::unordered_set<SurfaceData::SurfaceTag> tagsInUse;

        for (const TerrainPhysicsSurfaceMaterialMapping& mapping : m_configuration.m_surfaceMaterialMappings)
        {
            tagsInUse.insert(mapping.m_surfaceTag);
        }

        return AZStd::move(tagsInUse);
    }

    AZ::u32 EditorTerrainPhysicsColliderComponent::ConfigurationChanged()
    {
        m_component.UpdateConfiguration(m_configuration);
        UpdateConfigurationTagProvider();
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    void EditorTerrainPhysicsColliderComponent::UpdateConfigurationTagProvider()
    {
        for (TerrainPhysicsSurfaceMaterialMapping& mapping : m_configuration.m_surfaceMaterialMappings)
        {
            mapping.SetTagListProvider(this);
        }
    }

    AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> TerrainPhysicsSurfaceMaterialMapping::BuildSelectableTagList() const
    {
        return AZStd::move(Terrain::BuildSelectableTagList(m_tagListProvider, m_surfaceTag));
    }

    void TerrainPhysicsSurfaceMaterialMapping::SetTagListProvider(const EditorSurfaceTagListProvider* tagListProvider)
    {
        m_tagListProvider = tagListProvider;
    }
}
