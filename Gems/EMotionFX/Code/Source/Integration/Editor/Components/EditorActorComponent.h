/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Visibility/BoundsBus.h>

#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>

#include <Integration/Components/ActorComponent.h>
#include <Integration/Rendering/RenderActorInstance.h>

#include <LmbrCentral/Rendering/MaterialAsset.h>

#include <EMotionFX/Source/ActorBus.h>

namespace EMotionFX
{
    namespace Integration
    {
        class EditorActorComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , private AZ::Data::AssetBus::Handler
            , private AZ::TransformNotificationBus::Handler
            , private AZ::TickBus::Handler
            , private ActorComponentRequestBus::Handler
            , private EditorActorComponentRequestBus::Handler
            , private LmbrCentral::AttachmentComponentNotificationBus::Handler
            , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
            , private AzToolsFramework::EditorVisibilityNotificationBus::Handler
            , public AzFramework::BoundsRequestBus::Handler
        {
        public:
            AZ_EDITOR_COMPONENT(EditorActorComponent, "{A863EE1B-8CFD-4EDD-BA0D-1CEC2879AD44}");

            EditorActorComponent();
            ~EditorActorComponent() override;

            // AZ::Component overrides ...
            void Init() override;
            void Activate() override;
            void Deactivate() override;

            // ActorComponentRequestBus overrides ...
            ActorInstance* GetActorInstance() override { return m_actorInstance.get(); }
            bool GetRenderCharacter() const override;
            void SetRenderCharacter(bool enable) override;
            bool GetRenderActorVisible() const override;
            size_t GetNumJoints() const override;
            SkinningMethod GetSkinningMethod() const override;
            void SetActorAsset(AZ::Data::Asset<ActorAsset> actorAsset) override;

            // EditorActorComponentRequestBus overrides ...
            const AZ::Data::AssetId& GetActorAssetId() override;
            AZ::EntityId GetAttachedToEntityId() const override;

            // EditorVisibilityNotificationBus overrides ...
            void OnEntityVisibilityChanged(bool visibility) override;

            // EditorComponentSelectionRequestsBus overrides ...
            AZ::Aabb GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& viewportInfo) override;
            bool EditorSelectionIntersectRayViewport(
                const AzFramework::ViewportInfo& viewportInfo,
                const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) override;
            bool SupportsEditorRayIntersect() override { return true; }

            // AZ::Data::AssetBus::Handler overrides ...
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            // BoundsRequestBus overrides ...
            AZ::Aabb GetWorldBounds() override;
            AZ::Aabb GetLocalBounds() override;

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                ActorComponent::GetProvidedServices(provided);
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                ActorComponent::GetIncompatibleServices(incompatible);
            }

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
                ActorComponent::GetDependentServices(dependent);
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                ActorComponent::GetRequiredServices(required);
            }

            void SetRenderFlag(ActorRenderFlagBitset renderFlags);

            static void Reflect(AZ::ReflectContext* context);

        private:
            // Property callbacks.
            AZ::Crc32 OnAssetSelected();
            void OnMaterialChanged();
            void OnMaterialPerActorChanged();
            void OnLODLevelChanged();
            void OnDebugDrawFlagChanged();
            void OnSkinningMethodChanged();
            AZ::Crc32 OnAttachmentTypeChanged();
            AZ::Crc32 OnAttachmentTargetChanged();
            AZ::Crc32 OnAttachmentTargetJointSelect();
            void OnBBoxConfigChanged();
            bool AttachmentTargetVisibility();
            bool AttachmentTargetJointVisibility();
            AZStd::string AttachmentJointButtonText();
            void InitializeMaterial(ActorAsset& actorAsset);

            void LaunchAnimationEditor(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType&);

            // AZ::TransformNotificationBus overrides ...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // Called at edit-time when creating the component directly from an asset.
            void SetPrimaryAsset(const AZ::Data::AssetId& assetId) override;

            // AZ::TickBus::Handler
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            // LmbrCentral::AttachmentComponentNotificationBus::Handler
            void OnAttached(AZ::EntityId targetId) override;
            void OnDetached(AZ::EntityId targetId) override;

            void CheckActorCreation();
            void BuildGameEntity(AZ::Entity* gameEntity) override;

            void LoadActorAsset();
            void DestroyActorInstance();

            bool IsValidAttachment(const AZ::EntityId& attachment, const AZ::EntityId& attachTo) const;

            bool IsAtomDisabled() const;

            AZ::Data::Asset<ActorAsset>         m_actorAsset;               ///< Assigned actor asset.
            AZStd::vector<AZ::EntityId>         m_attachments;              ///< A list of entities that are attached to this entity.
            bool                                m_renderSkeleton;           ///< Toggles rendering of character skeleton.
            bool                                m_renderCharacter;          ///< Toggles rendering of character model.
            bool                                m_renderBounds;             ///< Toggles rendering of the world bounding box.
            bool                                m_entityVisible;            ///< Entity visible from the EditorVisibilityNotificationBus
            SkinningMethod                      m_skinningMethod;           ///< The skinning method for this actor
            AttachmentType                      m_attachmentType;           ///< Attachment type.
            AZ::EntityId                        m_attachmentTarget;         ///< Target entity to attach to, if any.
            AZStd::string                       m_attachmentJointName;      ///< Joint name on target to which to attach (if ActorAttachment).
            size_t                              m_attachmentJointIndex;
            size_t                              m_lodLevel;
            ActorComponent::BoundingBoxConfiguration m_bboxConfig;
            bool                                m_forceUpdateJointsOOV = false;
            ActorRenderFlagBitset                 m_debugRenderFlags;         ///< Actor debug render flag
            // \todo attachmentTarget node nr

            // Note: LOD work in progress. For now we use one material instead of a list of material, because we don't have the support for LOD with multiple scene files.
            // We purposely kept a materialList in actorComponent and actorRenderNode for the flexibility in future.
            // At the moment, the materialList stores duplicates of the same material.
            AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset>   m_materialPerActor;
            ActorAsset::MaterialList            m_materialPerLOD;           ///< Material assignment for each LOD level.

            ActorAsset::ActorInstancePtr        m_actorInstance;            ///< Live actor instance.
            AZStd::unique_ptr<RenderActorInstance> m_renderActorInstance;
        };
    } // namespace Integration
} // namespace EMotionFX
