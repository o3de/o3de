/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzFramework/Spawnable/Spawnable.h>

namespace AzFramework
{
    //! Notifications send when the root spawnable updates. Events will always be called from the main thread.
    class RootSpawnableNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~RootSpawnableNotifications() = default;

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const bool EnableEventQueue = true;
        using MutexType = AZStd::recursive_mutex;
        
        //! Called when the root spawnable has been assigned a new value. This may be called several times without a call to release
        //!     in between.
        //! @note: The callback is not queued but immediately called from a random thread. This is done because this callback is typically
        //!     used before entities are spawned and if it's queued then the entities spawn before this callback is called.
        //! @param rootSpawnable The new root spawnable that was assigned.
        //! @param generation The generation of the root spawnable. This will increment every time a new spawnable is assigned.
        virtual void OnRootSpawnableAssigned([[maybe_unused]] AZ::Data::Asset<Spawnable> rootSpawnable,
            [[maybe_unused]] uint32_t generation) {}
        //! Called when the root spawnable has completed spawning of entities. This may be called several times without a call to release
        //!     in between.
        //! @note: This callback is queued and will be called with a delay and from the main thread.
        //! @param rootSpawnable The new root spawnable that was used to spawn entities from.
        //! @param generation The generation of the root spawnable. This will increment every time a new spawnable is assigned.
        virtual void OnRootSpawnableReady(
            [[maybe_unused]] AZ::Data::Asset<Spawnable> rootSpawnable, [[maybe_unused]] uint32_t generation) {}
        //! Called when the root spawnable has Released. This will only be called if there's no root spawnable assigned to take the
        //!     place of the original root spawnable.
        //! Note: This callback is queued and will be called with a delay and from the main thread.
        //! @param generation The generation of the root spawnable that was released.
        virtual void OnRootSpawnableReleased([[maybe_unused]] uint32_t generation) {}
    };

    using RootSpawnableNotificationBus = AZ::EBus<RootSpawnableNotifications>;

    //! Interface to manage the root spawnable. All calls to this interface need to be made
    //! from the main thread and all events are called from the main thread.
    class RootSpawnableDefinition
    {
    public:
        AZ_RTTI(AzFramework::RootSpawnableDefinition, "{6F3698F4-005D-4F26-BE99-5DC2E21FAD38}");

        using OnRootSpawnableReadyEvent = AZ::Event < const AZ::Data::Asset<Spawnable>&, bool>;

        //! Sets the provided spawnable as the new root spawnable. If a root spawnable has already
        //! been assigned this will unload any entities spawned from it and replace it. The provided
        //! spawnable will become the new root and all entities in it will be instanced into the
        //! game entity context.
        //! @return the generation of the root spawnable that has been assigned.
        virtual uint64_t AssignRootSpawnable(AZ::Data::Asset<Spawnable> rootSpawnable) = 0;
        //! Releases the root spawnable if one is set, resulting in all entities spawned from it to
        //! be deleted and the spawnable asset to be released. This call is automatically done when
        //! AssignRootSpawnable is called while a root spawnable is assigned.
        virtual void ReleaseRootSpawnable() = 0;
        //! Force processing all SpawnableEntitiesManager requests immediately
        //! This is useful when loading a different level while SpawnableEntitiesManager still has
        //! pending requests
        virtual void ProcessSpawnableQueue() = 0;
        //! Force processing all SpawnableEntitiesManager requests immediately and keep reprocessing until the queue is empty
        //! This is useful when unloading and shutting down to ensure that all pending requests are cleared.
        virtual void ProcessSpawnableQueueUntilEmpty() = 0;
    };

    using RootSpawnableInterface = AZ::Interface<RootSpawnableDefinition>;
} // namespace AzFramework
