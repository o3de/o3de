/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzFramework/Physics/Ragdoll.h>

namespace AzFramework
{
    class CharacterPhysicsDataRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~CharacterPhysicsDataRequests() = default;

        virtual bool GetRagdollConfiguration(Physics::RagdollConfiguration& config) const = 0;
        virtual Physics::RagdollState GetBindPose(const Physics::RagdollConfiguration& config) const = 0;
        virtual AZStd::string GetParentNodeName(const AZStd::string& childName) const = 0;
    };

    using CharacterPhysicsDataRequestBus = AZ::EBus<CharacterPhysicsDataRequests>;

    class CharacterPhysicsDataNotifications
        : public AZ::ComponentBus
    {
    public:
        virtual ~CharacterPhysicsDataNotifications() = default;

        virtual void OnRagdollConfigurationReady(const Physics::RagdollConfiguration& ragdollConfiguration) = 0;

        //! When connecting to this bus, if the ragdoll configuration is ready
        //! it will immediately send an OnRagdollConfigurationReady event.
        template<class Bus>
        struct ConnectionPolicy
            : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(
                typename Bus::BusPtr& busPtr,
                typename Bus::Context& context,
                typename Bus::HandlerNode& handler,
                typename Bus::Context::ConnectLockGuard& connectLock,
                const typename Bus::BusIdType& id = 0)
            {
                AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                bool ragdollConfigValid = false;
                Physics::RagdollConfiguration ragdollConfiguration;
                CharacterPhysicsDataRequestBus::EventResult(ragdollConfigValid, id,
                    &CharacterPhysicsDataRequests::GetRagdollConfiguration, ragdollConfiguration);
                if (ragdollConfigValid)
                {
                    handler->OnRagdollConfigurationReady(ragdollConfiguration);
                }
            }
        };
    };

    using CharacterPhysicsDataNotificationBus = AZ::EBus<CharacterPhysicsDataNotifications>;
} // namespace AzFramework
