/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        //! Bus for retrieving data about a given entity's Atom mesh.
        class AtomMeshRequests
        : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
            using BusIdType = EntityId;

            //! Returns the handle to the Atom mesh.
            virtual const MeshFeatureProcessorInterface::MeshHandle* GetMeshHandle() const = 0;
        };

        using AtomMeshRequestBus = EBus<AtomMeshRequests>;
        
        //! Bus for receiving notifications about a given entity's Atom mesh state.
        class AtomMeshNotifications
        : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
            using BusIdType = EntityId;

            //! Notification for when the Atom mesh handle has been acquired (and thus ready for use).
            virtual void OnAcquireMesh(const MeshFeatureProcessorInterface::MeshHandle* meshHandle) = 0;

            //! When connecting to this bus, if the handle is ready you will immediately get an OnMeshHandleAcquire notification.
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

                    // If this entity has no AtomMeshRequests handler treat this call as a no-op.
                    if(!AtomMeshRequestBus::HasHandlers(id))
                    {
                        return;
                    }

                    const MeshFeatureProcessorInterface::MeshHandle* meshHandle;
                    AtomMeshRequestBus::EventResult(meshHandle, id, &AtomMeshRequestBus::Events::GetMeshHandle);

                    if (meshHandle && meshHandle->IsValid())
                    {
                        handler->OnAcquireMesh(meshHandle);
                    }
                }
            };
        };

        using AtomMeshNotificationBus = EBus<AtomMeshNotifications>;
    } // namespace Render
} // namespace Render
