/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/Component/TransformBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Visibility/BoundsBus.h>

#include <AzFramework/Render/GeometryIntersectionBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <LmbrCentral/Rendering/RenderNodeBus.h>
#include <LmbrCentral/Rendering/RenderBoundsBus.h>

#include "MeshComponent.h"


struct IPhysicalEntity;

namespace LmbrCentral
{
    /**
     * In-editor mesh component.
     * Conducts some additional listening and operations to ensure immediate
     * effects when changing fields in the editor.
     */
    class EditorMeshComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public AZ::Data::AssetBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
        , public AzFramework::BoundsRequestBus::Handler
        , private AzFramework::RenderGeometry::IntersectionRequestBus::Handler
        , private MeshComponentRequestBus::Handler
        , private MaterialOwnerRequestBus::Handler
        , private MeshComponentNotificationBus::Handler
        , private RenderNodeRequestBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private AzToolsFramework::EditorVisibilityNotificationBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private LegacyMeshComponentRequestBus::Handler
        , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , private AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler
    {
    public:
        AZ_COMPONENT(EditorMeshComponent, "{FC315B86-3280-4D03-B4F0-5553D7D08432}", AzToolsFramework::Components::EditorComponentBase)

        ~EditorMeshComponent() = default;

        const float s_renderNodeRequestBusOrder = 100.f;

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // BoundsRequestBus and MeshComponentRequestBus overrides ...
        AZ::Aabb GetWorldBounds() override;
        AZ::Aabb GetLocalBounds() override;

        // IntersectionRequestBus overrides ...
        AzFramework::RenderGeometry::RayResult RenderGeometryIntersect(const AzFramework::RenderGeometry::RayRequest& ray) override;

        // MeshComponentRequestBus overrides ...
        void SetMeshAsset(const AZ::Data::AssetId& id) override;
        AZ::Data::Asset<AZ::Data::AssetData> GetMeshAsset() override { return m_mesh.GetMeshAsset(); }
        void SetVisibility(bool visible) override;
        bool GetVisibility() override;

        // MaterialOwnerRequestBus overrides ...
        void SetMaterial(_smart_ptr<IMaterial>) override;
        _smart_ptr<IMaterial> GetMaterial() override;

        // MeshComponentNotificationBus overrides ...
        void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
        void OnMeshDestroyed() override;

        // RenderNodeRequestBus overrides ...
        IRenderNode* GetRenderNode() override;
        float GetRenderNodeRequestBusOrder() const override;

        // TransformNotificationBus overrides ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        void OnStaticChanged(bool isStatic) override;

        // EditorVisibilityNotificationBus overrides ...
        void OnEntityVisibilityChanged(bool visibility) override;

        // AzFramework::EntityDebugDisplayEventBus overrides ....
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        //! Called when you want to change the game asset through code (like when creating components based on assets).
        void SetPrimaryAsset(const AZ::Data::AssetId& assetId) override;

        // LegacyMeshComponentRequests overrides ...
        IStatObj* GetStatObj() override;

        // EditorComponentBase overrides ...
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // AZ::Data::AssetBus overrides ...
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // AzFramework::AssetCatalogEventBus overrides ...
        void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;

        // EditorComponentSelectionRequestsBus overrides ...
        AZ::Aabb GetEditorSelectionBoundsViewport(
            const AzFramework::ViewportInfo& viewportInfo) override;
        bool EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) override;
        bool SupportsEditorRayIntersect() override { return true; }

        // EditorComponentSelectionNotificationsBus overrides ...
        void OnAccentTypeChanged(AzToolsFramework::EntityAccentType accent) override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("MeshService", 0x71d8a455));
            provided.push_back(AZ_CRC("LegacyMeshService", 0xb462a299));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("EditorVisibilityService", 0x90888caf));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("MeshService", 0x71d8a455));
            incompatible.push_back(AZ_CRC("LegacyMeshService", 0xb462a299));
        }

        static void Reflect(AZ::ReflectContext* context);

    protected:

        // Decides if this mesh affects the navmesh or not.
        void AffectNavmesh();

        AZStd::string GetMeshViewportIconPath() const;

        AzToolsFramework::EntityAccentType m_accentType = AzToolsFramework::EntityAccentType::None; ///< State of the entity selection in the viewport.
        MeshComponentRenderNode m_mesh; ///< IRender node implementation.

        AzFramework::EntityContextId m_contextId;
        AZ::Vector3 m_debugPos = AZ::Vector3(0);
        AZ::Vector3 m_debugNormal = AZ::Vector3(0);
    };

    // Helper function useful for automation.
    bool AddMeshComponentWithMesh(const AZ::EntityId& targetEntity, const AZ::Uuid& meshAssetId);
} // namespace LmbrCentral
