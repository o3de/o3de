/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LmbrCentral/Animation/AttachmentComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Transform.h>

struct ISkeletonPose;

namespace AZ
{
    namespace Render
    {
        /*!
         * Configuration data for AttachmentComponent.
         */
        struct AttachmentConfiguration
        {
            AZ_TYPE_INFO(AttachmentConfiguration, "{74B5DC69-DE44-4640-836A-55339E116795}");

            virtual ~AttachmentConfiguration() = default;

            static void Reflect(AZ::ReflectContext* context);

            //! Attach to this entity.
            AZ::EntityId m_targetId;

            //! Attach to this bone on target entity.
            AZStd::string m_targetBoneName;

            //! Offset from target.
            AZ::Transform m_targetOffset = AZ::Transform::Identity();

            //! Whether to attach to target upon activation.
            //! If false, the entity remains detached until Attach() is called.
            bool m_attachedInitially = true;

            //! Source from which to retrieve scale information.
            enum class ScaleSource : AZ::u8
            {
                WorldScale,         // Scaled in world space.
                TargetEntityScale,  // Adopt scaling of attachment target entity.
                TargetBoneScale,    // Adopt scaling of attachment target entity/joint.
            };
            ScaleSource m_scaleSource = ScaleSource::WorldScale;
        };

        /*
         * Common functionality for game and editor attachment components.
         * The BoneFollower tracks movement of the target's bone and
         * updates the owning entity's TransformComponent to follow.
         * This class should be a member within the attachment component
         * and be activated/deactivated along with the component.
         * \ref AttachmentComponent
         */
        class BoneFollower
            : public LmbrCentral::AttachmentComponentRequestBus::Handler
            , public AZ::TransformNotificationBus::Handler
            , public AZ::Render::MeshComponentNotificationBus::Handler
            , public AZ::Data::AssetBus::Handler
            , public AZ::TickBus::Handler
        {
        public:
            void Activate(AZ::Entity* owner, const AttachmentConfiguration& initialConfiguration, bool targetCanAnimate);
            void Deactivate();

            ////////////////////////////////////////////////////////////////////////
            // AttachmentComponentRequests
            void Reattach(bool detachFirst) override;
            void Attach(AZ::EntityId targetId, const char* targetBoneName, const AZ::Transform& offset) override;
            void Detach() override;
            void SetAttachmentOffset(const AZ::Transform& offset) override;
            const char* GetJointName() override;
            AZ::EntityId GetTargetEntityId() override;
            AZ::Transform GetOffset() override;
            ////////////////////////////////////////////////////////////////////////

        private:

            ////////////////////////////////////////////////////////////////////////
            // AZ::TickBus
            //! Check target bone transform every frame.
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            //! Make sure target bone transform updates after animation update.
            int GetTickOrder() override;
            ////////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////////
            // AZ::TransformNotificationBus
            //! When target's transform changes
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
            ////////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////////
            // MeshComponentEvents
            //! When target's mesh changes
            void OnModelReady(const AZ::Data::Asset<AZ::RPI::ModelAsset>& modelAsset, const AZ::Data::Instance<AZ::RPI::Model>& model) override;
            ////////////////////////////////////////////////////////////////////////

            void BindTargetBone();

            AZ::Transform QueryBoneTransform() const;

            void UpdateOwnerTransformIfNecessary();

            //! Entity which which is being attached.
            AZ::EntityId    m_ownerId;

            //! Whether to query bone position per-frame (false while in editor)
            bool            m_targetCanAnimate = false;

            AZ::EntityId    m_targetId;
            AZStd::string   m_targetBoneName;
            AZ::Transform   m_targetOffset; //!< local transform
            AZ::Transform   m_targetBoneTransform; //!< local transform of bone
            AZ::Transform   m_targetEntityTransform; //!< world transform of target
            bool            m_isTargetEntityTransformKnown = false;

            //! Cached value, so we don't update owner's position unnecessarily.
            AZ::Transform   m_cachedOwnerTransform;
            bool            m_isUpdatingOwnerTransform = false; //!< detect infinite loops when updating owner's transform

            // Cached character values to avoid repeated lookup.
            // These are set by calling ResetCharacter()
            int             m_targetBoneId; //!< negative when bone not found

            AttachmentConfiguration::ScaleSource m_scaleSource;
        };

        /*!
         * The AttachmentComponent lets an entity stick to a particular bone on
         * a target entity. This is achieved by tracking movement of the target's
         * bone and updating the entity's TransformComponent accordingly.
         */
        class AttachmentComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(AttachmentComponent, "{2D17A64A-7AC5-4C02-AC36-C5E8141FFDDF}");

            friend class EditorAttachmentComponent;

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC("AttachmentService", 0x5aaa7b63));
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC("AttachmentService", 0x5aaa7b63));
                incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
            }

            ~AttachmentComponent() override = default;

        private:
            ////////////////////////////////////////////////////////////////////////
            // AZ::Component
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////////

            //! Initial configuration for m_attachment
            AttachmentConfiguration m_initialConfiguration;

            //! Implements actual attachment functionality
            BoneFollower m_boneFollower;
        };
    } // namespace Render
} // namespace AZ
