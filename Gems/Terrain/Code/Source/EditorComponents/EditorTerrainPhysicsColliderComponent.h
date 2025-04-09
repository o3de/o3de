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
#include <EditorSurfaceTagListProvider.h>

namespace Terrain
{
    class EditorTerrainPhysicsColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public EditorSurfaceTagListProvider
    {
    public:
        AZ_EDITOR_COMPONENT(EditorTerrainPhysicsColliderComponent, "{C43FAB8F-3968-46A6-920E-E84AEDED3DF5}", AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services);

        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // EditorComponentBase overrides...
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        static constexpr auto s_categoryName = "Terrain";
        static constexpr auto s_componentName = "Terrain Physics Heightfield Collider";
        static constexpr auto s_componentDescription = "Provides terrain data to a physics collider in the form of a heightfield and surface->material mapping.";
        static constexpr auto s_icon = "Editor/Icons/Components/TerrainPhysicsCollider.svg";
        static constexpr auto s_viewportIcon = "Editor/Icons/Components/Viewport/TerrainPhysicsCollider.svg";
        static constexpr auto s_helpUrl = "";

    private:
        // EditorSurfaceTagListProvider interface implementation
        AZStd::unordered_set<SurfaceData::SurfaceTag> GetSurfaceTagsInUse() const override;

        AZ::u32 ConfigurationChanged();
        void UpdateConfigurationTagProvider();

        TerrainPhysicsColliderConfig m_configuration;
        TerrainPhysicsColliderComponent m_component;
    };
}
