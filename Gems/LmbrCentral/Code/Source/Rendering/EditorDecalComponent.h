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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include "DecalComponent.h"

namespace LmbrCentral
{
    /*!
     * Extends DecalConfiguration structure to add editor functionality
     * such as property handlers and visibility filters, as well as
     * reflection for editing.
     */
    class EditorDecalConfiguration
        : public DecalConfiguration
    {
    public:

        AZ_TYPE_INFO(EditorDecalConfiguration, "{559556BE-F41E-43C0-9EE6-7048D84D7952}");

        ~EditorDecalConfiguration() override {}

        static void Reflect(AZ::ReflectContext* context);

        AZ::u32 MajorPropertyChanged() override;
        AZ::u32 MinorPropertyChanged() override;
    };

    /*!
     * In-editor decal component.
     * Handles placement of decals in editor.
     */
    class EditorDecalComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private DecalComponentEditorRequests::Bus::Handler
        , private MaterialOwnerRequestBus::Handler
        , private RenderNodeRequestBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AzToolsFramework::EditorVisibilityNotificationBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , public AzFramework::BoundsRequestBus::Handler
    {
    private:
        using Base = AzToolsFramework::Components::EditorComponentBase;

    public:
        AZ_COMPONENT(EditorDecalComponent, "{BA3890BD-D2E7-4DB6-95CD-7E7D5525567A}", AzToolsFramework::Components::EditorComponentBase);

        EditorDecalComponent();
        ~EditorDecalComponent() override;

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        // AzToolsFramework::EditorVisibilityNotificationBus interface implementation
        void OnEntityVisibilityChanged(bool visibility) override;

        // DecalComponentEditorRequests::Bus::Handler interface implementation
        void RefreshDecal() override;

        // MaterialOwnerRequestBus interface implementation
        void SetMaterial(_smart_ptr<IMaterial>) override;
        _smart_ptr<IMaterial> GetMaterial() override;

        // RenderNodeRequestBus::Handler interface implementation
        IRenderNode* GetRenderNode() override;
        float GetRenderNodeRequestBusOrder() const override;

        // TransformNotificationBus::Handler interface implementation
        void OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/) override;

        // EditorComponentSelectionRequestsBus::Handler interface implementation
        AZ::Aabb GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo&) override;
        bool SupportsEditorRayIntersect() override { return true; };
        bool EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src, const AZ::Vector3& dir,
            float& distance) override;

        // BoundsRequestBus overrides ...
        AZ::Aabb GetWorldBounds() override;
        AZ::Aabb GetLocalBounds() override;

        void BuildGameEntity(AZ::Entity* gameEntity) override;

        //! Called when you want to change the game asset through code (like when creating components based on assets).
        void SetPrimaryAsset(const AZ::Data::AssetId& assetId) override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            DecalComponent::GetProvidedServices(provided);
        }
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            DecalComponent::GetDependentServices(dependent);
            dependent.push_back(AZ_CRC_CE("EditorVisibilityService"));
        }
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            DecalComponent::GetRequiredServices(required);
        }

        static void Reflect(AZ::ReflectContext* context);

    protected:

        EditorDecalConfiguration m_configuration;

        IDecalRenderNode* m_decalRenderNode;
        uint32 m_renderFlags;

        uint8 m_materialLayersMask;

    private:
        // AzToolsFramework::EditorEvents interface implementation
        void OnEditorSpecChange() override;
    };
} // namespace LmbrCentral
