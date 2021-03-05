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

#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>

#include <LmbrCentral/Rendering/RenderNodeBus.h>
#include <LmbrCentral/Rendering/MaterialOwnerBus.h>
#include <LmbrCentral/Rendering/RenderBoundsBus.h>

#include "SkinnedMeshComponent.h"

struct IPhysicalEntity;

namespace LmbrCentral
{
    /*!
    * In-editor mesh component.
    * Conducts some additional listening and operations to ensure immediate
    * effects when changing fields in the editor.
    */
    class EditorSkinnedMeshComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AzFramework::BoundsRequestBus::Handler
        , private MeshComponentRequestBus::Handler
        , private MaterialOwnerRequestBus::Handler
        , private MeshComponentNotificationBus::Handler
        , private RenderNodeRequestBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private AzToolsFramework::EditorVisibilityNotificationBus::Handler
        , private SkeletalHierarchyRequestBus::Handler
        , public AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
    {
    public:

        AZ_COMPONENT(EditorSkinnedMeshComponent, "{D3E1A9FC-56C9-4997-B56B-DA186EE2D62A}", AzToolsFramework::Components::EditorComponentBase);

        ~EditorSkinnedMeshComponent() override = default;

        const float s_renderNodeRequestBusOrder = 100.f;

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // BoundsRequestBus and MeshComponentRequestBus overrides ...
        AZ::Aabb GetWorldBounds() override;
        AZ::Aabb GetLocalBounds() override;

        // MeshComponentRequestBus overrides ...
        void SetMeshAsset(const AZ::Data::AssetId& id) override;
        AZ::Data::Asset<AZ::Data::AssetData> GetMeshAsset() override { return m_mesh.GetMeshAsset(); }
        void SetVisibility(bool newVisibility) override;
        bool GetVisibility() override;

        // SkeletalHierarchyRequestBus overrides ...
        AZ::u32 GetJointCount() override;
        const char* GetJointNameByIndex(AZ::u32 jointIndex) override;
        AZ::s32 GetJointIndexByName(const char* jointName) override;
        AZ::Transform GetJointTransformCharacterRelative(AZ::u32 jointIndex) override;

        // MaterialOwnerRequestBus interface implementation
        void SetMaterial(_smart_ptr<IMaterial>) override;
        _smart_ptr<IMaterial> GetMaterial() override;

        // MeshComponentNotificationBus interface implementation
        void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
        void OnMeshDestroyed() override;

        // RenderNodeRequestBus::Handler
        IRenderNode* GetRenderNode() override;
        float GetRenderNodeRequestBusOrder() const override;

        // TransformBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // EditorVisibilityNotificationBus
        void OnEntityVisibilityChanged(bool visibility) override;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // EditorComponentSelectionRequestsBus::Handler
        AZ::Aabb GetEditorSelectionBoundsViewport(
            const AzFramework::ViewportInfo& viewportInfo) override;

        //! Called when you want to change the game asset through code (like when creating components based on assets).
        void SetPrimaryAsset(const AZ::Data::AssetId& assetId) override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("MeshService", 0x71d8a455));
            provided.push_back(AZ_CRC("SkinnedMeshService", 0xac7cea96));
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
            incompatible.push_back(AZ_CRC("SkinnedMeshService", 0xac7cea96));
        }

        static void Reflect(AZ::ReflectContext* context);

    protected:

        // Editor-specific physicalization for the attached mesh. This is needed to support
        // features in the editor that rely on edit-time collision info (i.e. object snapping).
        void CreateEditorPhysics();
        void DestroyEditorPhysics();

        SkinnedMeshComponentRenderNode m_mesh;     ///< IRender node implementation

        AZ::Transform m_physTransform;      ///< To track scale changes, which requires re-physicalizing.
    };

} // namespace LmbrCentral
