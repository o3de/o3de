/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AttachmentComponent.h"
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/Entity.h>
#include <MathConversion.h>
#include <LmbrCentral/Rendering/MeshAsset.h>
#include <LmbrCentral/Animation/AttachmentComponentBus.h>
#include <LmbrCentral/Animation/SkeletalHierarchyRequestBus.h>

namespace AZ
{
    namespace Render
    {
        /// Behavior Context handler for AttachmentComponentNotificationBus
        class BehaviorAttachmentComponentNotificationBusHandler : public LmbrCentral::AttachmentComponentNotificationBus::Handler,
                                                                  public AZ::BehaviorEBusHandler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER(
                BehaviorAttachmentComponentNotificationBusHandler, "{636B95A0-5C7D-4EE7-8645-955665315451}", AZ::SystemAllocator,
                OnAttached, OnDetached);

            void OnAttached(AZ::EntityId id) override
            {
                Call(FN_OnAttached, id);
            }

            void OnDetached(AZ::EntityId id) override
            {
                Call(FN_OnDetached, id);
            }
        };

        void AttachmentConfiguration::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<AttachmentConfiguration>()
                    ->Version(1)
                    ->Field("Target ID", &AttachmentConfiguration::m_targetId)
                    ->Field("Target Bone Name", &AttachmentConfiguration::m_targetBoneName)
                    ->Field("Target Offset", &AttachmentConfiguration::m_targetOffset)
                    ->Field("Attached Initially", &AttachmentConfiguration::m_attachedInitially)
                    ->Field("Scale Source", &AttachmentConfiguration::m_scaleSource);
            }
            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->EBus<LmbrCentral::AttachmentComponentRequestBus>("AttachmentComponentRequestBus")
                    ->Event("Attach", &LmbrCentral::AttachmentComponentRequestBus::Events::Attach)
                    ->Event("Detach", &LmbrCentral::AttachmentComponentRequestBus::Events::Detach)
                    ->Event("SetAttachmentOffset", &LmbrCentral::AttachmentComponentRequestBus::Events::SetAttachmentOffset);

                behaviorContext->EBus<LmbrCentral::AttachmentComponentNotificationBus>("AttachmentComponentNotificationBus")
                    ->Handler<BehaviorAttachmentComponentNotificationBusHandler>();
            }
        }

        void AttachmentComponent::Reflect(AZ::ReflectContext* context)
        {
            AttachmentConfiguration::Reflect(context);

            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<AttachmentComponent, AZ::Component>()->Version(1)->Field(
                    "Configuration", &AttachmentComponent::m_initialConfiguration);
            }
        }

        //=========================================================================
        // BoneFollower
        //=========================================================================

        void BoneFollower::Activate(AZ::Entity* owner, const AttachmentConfiguration& configuration, bool targetCanAnimate)
        {
            AZ_Assert(owner, "owner is required");
            AZ_Assert(!m_ownerId.IsValid(), "BoneFollower is already Activated");

            m_ownerId = owner->GetId();
            m_targetCanAnimate = targetCanAnimate;
            m_isUpdatingOwnerTransform = false;
            m_scaleSource = configuration.m_scaleSource;

            m_cachedOwnerTransform = AZ::Transform::CreateIdentity();
            EBUS_EVENT_ID_RESULT(m_cachedOwnerTransform, m_ownerId, AZ::TransformBus, GetWorldTM);

            if (configuration.m_attachedInitially)
            {
                Attach(configuration.m_targetId, configuration.m_targetBoneName.c_str(), configuration.m_targetOffset);
            }

            LmbrCentral::AttachmentComponentRequestBus::Handler::BusConnect(m_ownerId);
        }

        void BoneFollower::Deactivate()
        {
            AZ_Assert(m_ownerId.IsValid(), "BoneFollower was never Activated");

            LmbrCentral::AttachmentComponentRequestBus::Handler::BusDisconnect();
            Detach();
            m_ownerId.SetInvalid();
        }

        AZ::EntityId BoneFollower::GetTargetEntityId()
        {
            return m_targetId;
        }

        AZ::Transform BoneFollower::GetOffset()
        {
            return m_targetOffset;
        }

        void BoneFollower::Attach(AZ::EntityId targetId, const char* targetBoneName, const AZ::Transform& offset)
        {
            AZ_Assert(m_ownerId.IsValid(), "BoneFollower must be Activated to use.")

                // safe to try and detach, even if we weren't attached
                Detach();

            if (!targetId.IsValid())
            {
                return;
            }

            if (targetId == m_ownerId)
            {
                AZ_Error("Attachment Component", false, "AttachmentComponent cannot target itself");
                return;
            }

            // Note: the target entity may not be activated yet. That's ok.
            // When mesh is ready we are notified via MeshComponentEvents::OnModelReady
            // When transform is ready we are notified via TransformNotificationBus::OnTransformChanged

            m_targetId = targetId;
            m_targetBoneName = targetBoneName;
            m_targetOffset = offset;

            BindTargetBone();

            m_targetBoneTransform = AZ::Transform::Identity();

            m_isTargetEntityTransformKnown = false; // target's transform may not be available yet

            AZ::TransformBus::EventResult(
                m_cachedOwnerTransform, m_ownerId, &AZ::TransformBus::Events::GetWorldTM); // owner query will always succeed

            MeshComponentNotificationBus::Handler::BusConnect(m_targetId); // fires OnModelReady if asset is already ready
            AZ::TransformNotificationBus::Handler::BusConnect(m_targetId);
            if (m_targetCanAnimate)
            {
                // Only register for per-frame updates when target can animate
                AZ::TickBus::Handler::BusConnect();
            }

            // update owner's transform
            UpdateOwnerTransformIfNecessary();

            // alert others that we've attached
            LmbrCentral::AttachmentComponentNotificationBus::Event(m_targetId, &LmbrCentral::AttachmentComponentNotificationBus::Events::OnAttached, m_ownerId);
        }

        void BoneFollower::Detach()
        {
            AZ_Assert(m_ownerId.IsValid(), "BoneFollower must be Activated to use.");

            if (m_targetId.IsValid())
            {
                // alert others that we're detaching
                EBUS_EVENT_ID(m_targetId, LmbrCentral::AttachmentComponentNotificationBus, OnDetached, m_ownerId);

                MeshComponentNotificationBus::Handler::BusDisconnect();
                AZ::TransformNotificationBus::Handler::BusDisconnect(m_targetId);
                AZ::TickBus::Handler::BusDisconnect();

                m_targetId.SetInvalid();
            }
        }

        const char* BoneFollower::GetJointName()
        {
            return m_targetBoneName.c_str();
        }

        void BoneFollower::SetAttachmentOffset(const AZ::Transform& offset)
        {
            AZ_Assert(m_ownerId.IsValid(), "BoneFollower must be Activated to use.");

            if (m_targetId.IsValid())
            {
                m_targetOffset = offset;
                UpdateOwnerTransformIfNecessary();
            }
        }

        void BoneFollower::OnModelReady([[maybe_unused]] const AZ::Data::Asset<AZ::RPI::ModelAsset>& modelAsset, [[maybe_unused]] const AZ::Data::Instance<AZ::RPI::Model>& model)
        {
            // reset character values
            BindTargetBone();
            m_targetBoneTransform = QueryBoneTransform();

            // move owner if necessary
            UpdateOwnerTransformIfNecessary();
        }

        void BoneFollower::BindTargetBone()
        {
            m_targetBoneId = -1;
            LmbrCentral::SkeletalHierarchyRequestBus::EventResult(
                m_targetBoneId, m_targetId, &LmbrCentral::SkeletalHierarchyRequests::GetJointIndexByName, m_targetBoneName.c_str());
        }

        void BoneFollower::UpdateOwnerTransformIfNecessary()
        {
            // Can't update until target entity's transform is known
            if (!m_isTargetEntityTransformKnown)
            {
                if (AZ::TransformBus::GetNumOfEventHandlers(m_targetId) == 0)
                {
                    return;
                }

                AZ::TransformBus::EventResult(m_targetEntityTransform, m_targetId, &AZ::TransformBus::Events::GetWorldTM);
                m_isTargetEntityTransformKnown = true;
            }

            AZ::Transform finalTransform;
            if (m_scaleSource == AttachmentConfiguration::ScaleSource::WorldScale)
            {
                // apply offset in world-space
                finalTransform = m_targetEntityTransform * m_targetBoneTransform;
                finalTransform.SetUniformScale(1.0f);
                finalTransform *= m_targetOffset;
            }
            else if (m_scaleSource == AttachmentConfiguration::ScaleSource::TargetEntityScale)
            {
                // apply offset in target-entity-space (ignoring bone scale)
                AZ::Transform boneNoScale = m_targetBoneTransform;
                boneNoScale.SetUniformScale(1.0f);

                finalTransform = m_targetEntityTransform * boneNoScale * m_targetOffset;
            }
            else // AttachmentConfiguration::ScaleSource::TargetEntityScale
            {
                // apply offset in target-bone-space
                finalTransform = m_targetEntityTransform * m_targetBoneTransform * m_targetOffset;
            }

            if (m_cachedOwnerTransform != finalTransform)
            {
                AZ_Warning(
                    "Attachment Component", !m_isUpdatingOwnerTransform,
                    "AttachmentComponent detected a cycle when updating transform, do not target child entities.");
                if (!m_isUpdatingOwnerTransform)
                {
                    m_cachedOwnerTransform = finalTransform;
                    m_isUpdatingOwnerTransform = true;
                    EBUS_EVENT_ID(m_ownerId, AZ::TransformBus, SetWorldTM, finalTransform);
                    m_isUpdatingOwnerTransform = false;
                }
            }
        }

        AZ::Transform BoneFollower::QueryBoneTransform() const
        {
            AZ::Transform boneTransform = AZ::Transform::CreateIdentity();

            if (m_targetBoneId >= 0)
            {
                LmbrCentral::SkeletalHierarchyRequestBus::EventResult(
                    boneTransform, m_targetId, &LmbrCentral::SkeletalHierarchyRequests::GetJointTransformCharacterRelative, m_targetBoneId);
            }

            return boneTransform;
        }

        // fires when target's transform changes
        void BoneFollower::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
        {
            m_targetEntityTransform = world;
            m_isTargetEntityTransformKnown = true;
            UpdateOwnerTransformIfNecessary();
        }

        void BoneFollower::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
        {
            m_targetBoneTransform = QueryBoneTransform();
            UpdateOwnerTransformIfNecessary();
        }

        int BoneFollower::GetTickOrder()
        {
            return AZ::TICK_ATTACHMENT;
        }

        void BoneFollower::Reattach(bool detachFirst)
        {
#ifdef AZ_ENABLE_TRACING
            AZ::Entity* ownerEntity = nullptr;
            AZ::Entity* targetEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(ownerEntity, &AZ::ComponentApplicationBus::Events::FindEntity, m_ownerId);
            AZ::ComponentApplicationBus::BroadcastResult(targetEntity, &AZ::ComponentApplicationBus::Events::FindEntity, m_targetId);
            AZ_TracePrintf(
                "BoneFollower", "Reattaching entity '%s' to entity '%s'", ownerEntity ? ownerEntity->GetName().c_str() : "",
                targetEntity ? targetEntity->GetName().c_str() : "");
#endif

            if (m_targetId.IsValid() && detachFirst)
            {
                LmbrCentral::AttachmentComponentNotificationBus::Event(m_targetId, &LmbrCentral::AttachmentComponentNotificationBus::Events::OnDetached, m_ownerId);
            }

            if (m_targetId != m_ownerId)
            {
                LmbrCentral::AttachmentComponentNotificationBus::Event(m_targetId, &LmbrCentral::AttachmentComponentNotificationBus::Events::OnAttached, m_ownerId);
            }
        }

        //=========================================================================
        // AttachmentComponent
        //=========================================================================

        void AttachmentComponent::Activate()
        {
#ifdef AZ_ENABLE_TRACING
            bool isStaticTransform = false;
            AZ::TransformBus::EventResult(isStaticTransform, GetEntityId(), &AZ::TransformBus::Events::IsStaticTransform);
            AZ_Warning(
                "Attachment Component", !isStaticTransform, "Attachment needs to move, but entity '%s' %s has a static transform.",
                GetEntity()->GetName().c_str(), GetEntityId().ToString().c_str());
#endif

            m_boneFollower.Activate(GetEntity(), m_initialConfiguration, true);
        }

        void AttachmentComponent::Deactivate()
        {
            m_boneFollower.Deactivate();
        }
    } // namespace Render
} // namespace AZ
