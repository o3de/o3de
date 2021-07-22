/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    class Transform;
}

namespace LmbrCentral
{
    /*!
     * Messages serviced by the AttachmentComponent.
     * The AttachmentComponent lets an entity "stick" to a
     * particular bone on a target entity.
     */
    class AttachmentComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~AttachmentComponentRequests() = default;

        //! Change attachment target.
        //! The entity will detach from any previous target.
        //!
        //! \param targetId         Attach to this entity.
        //! \param targetBoneName   Attach to this bone on target entity.
        //!                         If targetBone is not found then attach to
        //!                         target entity's transform origin.
        //! \param offset           Attachment's offset from target.
        virtual void Attach(AZ::EntityId targetId, const char* targetBoneName, const AZ::Transform& offset) = 0;

        //! The entity will detach from its target.
        virtual void Detach() = 0;

        //! Trigger a detach followed by a re-attach using the currently setup targetId and bone name and offset.
        //! This can be used when an asset reloads for example.
        virtual void Reattach(bool detachFirst) = 0;

        //! Update entity's offset from target.
        virtual void SetAttachmentOffset(const AZ::Transform& offset) = 0;

        //! Get the selected joint name.
        virtual const char* GetJointName() = 0;

        //! Get the target entity Id.
        virtual AZ::EntityId GetTargetEntityId() = 0;

        //! Get the transform offset.
        virtual AZ::Transform GetOffset() = 0;
    };
    using AttachmentComponentRequestBus = AZ::EBus<AttachmentComponentRequests>;

    /*!
     * Events emitted by the AttachmentComponent.
     * The AttachmentComponent lets an entity "stick" to a
     * particular bone on a target entity.
     */
    class AttachmentComponentNotifications
        : public AZ::ComponentBus
    {
    public:
        template <class Bus>
        struct AttachmentNotificationConnectionPolicy
            : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock, const typename Bus::BusIdType& id = 0)
            {
                AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                AZ::EntityId targetId;
                AttachmentComponentRequestBus::EventResult(targetId, id, &AttachmentComponentRequestBus::Events::GetTargetEntityId);

                // Trigger a reattach, for cases where the other component didn't connect yet to this bus and never received the attach message.
                if (targetId.IsValid())
                {
                    AttachmentComponentRequestBus::Event(id, &AttachmentComponentRequestBus::Events::Reattach, false /* Skip dettaching */);
                }
            }
        };
        template<class Bus>
        using ConnectionPolicy = AttachmentNotificationConnectionPolicy<Bus>;

        virtual ~AttachmentComponentNotifications() = default;

        //! The entity has attached to the target.
        //! \param targetId The target being attached to.
        virtual void OnAttached(AZ::EntityId /*targetId*/) {};

        //! The entity is detaching from the target.
        //! \param targetId The target being detached from.
        virtual void OnDetached(AZ::EntityId /*targetId*/) {};
    };
    using AttachmentComponentNotificationBus = AZ::EBus<AttachmentComponentNotifications>;
} // namespace LmbrCentral
