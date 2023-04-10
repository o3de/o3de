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
#include <AzCore/Interface/Interface.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/RagdollPhysicsBus.h>
#include <AzFramework/Physics/CharacterPhysicsDataBus.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>

#include <Integration/Assets/ActorAsset.h>
#include <Integration/ActorComponentBus.h>
#include <Integration/Rendering/RenderActorInstance.h>

#include <EMotionFX/Source/ActorBus.h>
#include <LmbrCentral/Animation/AttachmentComponentBus.h>

namespace EMotionFX
{
    namespace Integration
    {
        class ActorComponent
            : public AZ::Component
            , private AZ::Data::AssetBus::Handler
            , private AZ::TransformNotificationBus::MultiHandler
            , private AZ::TickBus::Handler
            , private ActorComponentRequestBus::Handler
            , private ActorComponentNotificationBus::Handler
            , private LmbrCentral::AttachmentComponentNotificationBus::Handler
            , private AzFramework::CharacterPhysicsDataRequestBus::Handler
            , private AzFramework::RagdollPhysicsNotificationBus::Handler
            , private AzFramework::EntityDebugDisplayEventBus::Handler
        {
        public:
            AZ_COMPONENT(ActorComponent, "{BDC97E7F-A054-448B-A26F-EA2B5D78E377}");
            friend class EditorActorComponent;

            class BoundingBoxConfiguration
            {
            public:
                AZ_TYPE_INFO(BoundingBoxConfiguration, "{EBCFF975-00A5-4578-85C7-59909F52067C}");

                BoundingBoxConfiguration() = default;

                EMotionFX::ActorInstance::EBoundsType m_boundsType = EMotionFX::ActorInstance::BOUNDS_STATIC_BASED;
                float m_expandBy = 25.0f; ///< Expand the bounding volume by the given percentage.
                bool m_autoUpdateBounds    = true;
                float m_updateTimeFrequency = 0.0f;
                AZ::u32 m_updateItemFrequency = 1;

                // Set the bounding box configuration of the given actor instance to the parameters given by 'this'. The actor instance must not be null (this is not checked).
                void Set(ActorInstance* actorInstance) const;

                // Set the bounding box configuration, then update the bounds of the actor instance
                void SetAndUpdate(ActorInstance* actorInstance) const;

                static void Reflect(AZ::ReflectContext* context);

                AZ::Crc32 GetVisibilityAutoUpdate() const;
                AZ::Crc32 GetVisibilityAutoUpdateSettings() const;
            };

            /**
            * Configuration struct for procedural configuration of Actor Components.
            */
            struct Configuration
            {
                AZ_TYPE_INFO(Configuration, "{053BFBC0-ABAA-4F4E-911F-5320F941E1A8}")

                AZ::Data::Asset<ActorAsset> m_actorAsset{AZ::Data::AssetLoadBehavior::NoLoad}; ///< Selected actor asset.
                AZ::EntityId m_attachmentTarget{}; ///< Target entity this actor should attach to.
                size_t m_attachmentJointIndex = InvalidIndex; ///< Index of joint on target skeleton for actor attachments.
                AttachmentType m_attachmentType = AttachmentType::None; ///< Type of attachment.
                SkinningMethod m_skinningMethod = SkinningMethod::DualQuat; ///< The skinning method for this actor
                size_t m_lodLevel = 0;
                ActorRenderFlags m_renderFlags = ActorRenderFlags::Default; ///< Actor render flag

                // Force updating the joints when it is out of camera view. By
                // default, joints level update (beside the root joint) on
                // actor are disabled when the actor is out of view. 
                bool m_forceUpdateJointsOOV = false;
                BoundingBoxConfiguration m_bboxConfig; ///< Configuration for bounding box type and updates

                bool m_excludeFromReflectionCubeMaps = true;

                static void Reflect(AZ::ReflectContext* context);
            };

            ActorComponent(const Configuration* configuration = nullptr);
            ~ActorComponent() override;

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Activate() override;
            void Deactivate() override;

            //////////////////////////////////////////////////////////////////////////
            // ActorComponentRequestBus::Handler
            size_t GetNumJoints() const override;
            size_t GetJointIndexByName(const char* name) const override;
            AZ::Transform GetJointTransform(size_t jointIndex, Space space) const override;
            void GetJointTransformComponents(size_t jointIndex, Space space, AZ::Vector3& outPosition, AZ::Quaternion& outRotation, AZ::Vector3& outScale) const override;
            Physics::AnimationConfiguration* GetPhysicsConfig() const override;

            ActorInstance* GetActorInstance() override { return m_actorInstance.get(); }
            void AttachToEntity(AZ::EntityId targetEntityId, AttachmentType attachmentType) override;
            void DetachFromEntity() override;
            bool GetRenderCharacter() const override;
            void SetRenderCharacter(bool enable) override;
            bool GetRenderActorVisible() const override;
            SkinningMethod GetSkinningMethod() const override;
            void SetActorAsset(AZ::Data::Asset<ActorAsset> actorAsset) override;
            void EnableInstanceUpdate(bool enable) override;

            //////////////////////////////////////////////////////////////////////////
            // ActorComponentNotificationBus::Handler
            void OnActorInstanceCreated(ActorInstance* actorInstance) override;
            void OnActorInstanceDestroyed(ActorInstance* actorInstance) override;
            //////////////////////////////////////////////////////////////////////////

            // The entity has attached to the target.
            void OnAttached(AZ::EntityId targetId) override;

            // The entity is detaching from the target.
            void OnDetached(AZ::EntityId targetId) override;

            //////////////////////////////////////////////////////////////////////////
            // AzFramework::CharacterPhysicsDataBus::Handler
            bool GetRagdollConfiguration(Physics::RagdollConfiguration& config) const override;
            Physics::RagdollState GetBindPose(const Physics::RagdollConfiguration& config) const override;
            AZStd::string GetParentNodeName(const AZStd::string& childName) const override;

            //////////////////////////////////////////////////////////////////////////
            // AzFramework::RagdollPhysicsNotificationBus::Handler
            void OnRagdollActivated() override;
            void OnRagdollDeactivated() override;

            //////////////////////////////////////////////////////////////////////////
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC_CE("EMotionFXActorService"));
                provided.push_back(AZ_CRC_CE("MeshService"));
                provided.push_back(AZ_CRC_CE("CharacterPhysicsDataService"));
                provided.push_back(AZ_CRC_CE("MaterialConsumerService"));
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC_CE("EMotionFXActorService"));
                incompatible.push_back(AZ_CRC_CE("MeshService"));
                incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                required.push_back(AZ_CRC_CE("TransformService"));
            }

            static void Reflect(AZ::ReflectContext* context);

            // AZ::Data::AssetBus::Handler
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            bool IsPhysicsSceneSimulationFinishEventConnected() const;
            AZ::Data::Asset<ActorAsset> GetActorAsset() const { return m_configuration.m_actorAsset; }

            void SetRenderFlag(ActorRenderFlags renderFlags);

        private:
            // AZ::TransformNotificationBus::MultiHandler
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // AZ::TickBus::Handler
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            int GetTickOrder() override;

            // AzFramework::EntityDebugDisplayEventBus overrides ...
            void DisplayEntityViewport(
                const AzFramework::ViewportInfo& viewportInfo,
                AzFramework::DebugDisplayRequests& debugDisplay) override;

            void CheckActorCreation();
            void DestroyActor();
            void CheckAttachToEntity();

            Configuration                                   m_configuration;            ///< Component configuration.
            AZ::EntityId                                    m_attachmentTargetEntityId; ///< Target actor entity ID
            AZ::EntityId                                    m_attachmentPreviousParent; ///< The parent entity id before attaching to the attachment target.
            ActorAsset::ActorInstancePtr                    m_actorInstance;            ///< Live actor instance.
            AZStd::vector<AZ::EntityId>                     m_attachments;

            AZStd::unique_ptr<RenderActorInstance>          m_renderActorInstance;

            AzPhysics::SceneEvents::OnSceneSimulationFinishHandler m_sceneFinishSimHandler;
            bool m_processLoadedAsset = false;
        };
    } //namespace Integration
} // namespace EMotionFX
