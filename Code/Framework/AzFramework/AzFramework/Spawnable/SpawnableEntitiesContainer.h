/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <AzFramework/Spawnable/SpawnableMonitor.h>

namespace AZ
{
    class Entity;
}

namespace AzFramework
{
    //! A utility class to simplify the life-cycle management of entities created from a Spawnable.
    //! This class will keep track of the created entities and take appropriate action when the spawnable changes.
    //! Calls to this container should be done from the same thread, but multiple threads can create their own container for
    //! the same Spawnable. This container is for simple use cases. Complex situations can directly call the
    //! SpawnablesEntitesInterface and use the SpawnableMonitor if needed.
    class SpawnableEntitiesContainer
    {
    public:
        using AlertCallback = AZStd::function<void(uint32_t generation)>;

        enum class CheckIfSpawnableIsLoaded : bool
        {
            Yes,
            No
        };

        //! Constructs a new spawnables entity container that has not been connected.
        SpawnableEntitiesContainer() = default;
        //! Constructs a new spawnables entity container that connects to the provided spawnable.
        //! @param spawnable The Spawnable that will be monitored and used as a template to create entities from.
        explicit SpawnableEntitiesContainer(AZ::Data::Asset<Spawnable> spawnable);
        ~SpawnableEntitiesContainer();

        //! Returns true if the container has a spawnable set and can process requests, otherwise returns falls.
        [[nodiscard]] bool IsSet() const;
        //! Returns a number that identifies the current generation of the container with. The completion callback can still receive
        //!     calls from older generations as processing completes on those. The returned value can be used to help calls tell
        //!     older versions apart from newer ones.
        [[nodiscard]] uint32_t GetCurrentGeneration() const;

        //! Puts in a request to spawn entities using all entities in the provided spawnable as a template.
        //! @param optionalArgs optional arguments for spawning
        void SpawnAllEntities(SpawnAllEntitiesOptionalArgs optionalArgs = {});
        //! Puts in a request to spawn entities using the entities found in the spawnable at the provided indices as a template.
        //! @param entityIndices A list of indices to the entities in the spawnable.
        void SpawnEntities(AZStd::vector<uint32_t> entityIndices);
        //! Puts in a request to despawn all previous spawned entities.
        void DespawnAllEntities();

        //! Resets the spawnable and completion callback. This call will clear first if a spawnable has already been set. See
        //!     Clear for more details.
        //! @param spawnable The Spawnable that will be monitored and used as a template to create entities from.
        void Reset(AZ::Data::Asset<Spawnable> spawnable);
        //! Puts in a request to disconnect from the connected spawnable. This will immediately clear the internal
        //!     state to allow for a Reset, but the release of the spawnable itself will be delayed. As a result any pending and
        //!     in-flight requests will complete first. If a callback has been set it will be called one more time after this
        //!     function returns, possibly outliving the lifetime of the container.
        void Clear();

        //! Adds an alert that will trigger the provided callback once all previous calls to change the container have completed.
        //! This includes calls to (de)spawn entities, reset the container or clear. The callback can be called from threads
        //! other than the calling thread including the main thread. Note that because the alert is queued it can still be called
        //! after the container has been deleted or can be called for a previously assigned spawnable. In the latter case check
        //! if the current generation matches the generation provided with the callback.
        //! @param callback The function called when the alert triggers. This can be called from a different thread than the one that
        //!     the one that made the call to Alert.
        //! @param checkSpawnableIsLoaded If true the alert will also block until the spawnable has been loaded. If false then it will
        //!     be called after all previous calls have completed, but the spawnable may not be loaded at that point.
        void Alert(AlertCallback callback, CheckIfSpawnableIsLoaded spawnableCheck = CheckIfSpawnableIsLoaded::No);

    private:
        void Connect(AZ::Data::Asset<Spawnable> spawnable);

        struct ThreadSafeData
        {
            AZ_CLASS_ALLOCATOR(SpawnableEntitiesContainer::ThreadSafeData, AZ::SystemAllocator);
            EntitySpawnTicket m_spawnedEntitiesTicket;
            uint32_t m_generation{ 0 };
        };

        class Monitor final : public SpawnableMonitor
        {
        public:
            AZStd::shared_ptr<ThreadSafeData> m_threadData;

        protected:
            void OnSpawnableReloaded(AZ::Data::Asset<Spawnable>&& replacementAsset) override;
        };

        Monitor m_monitor;
        AZStd::atomic_uint32_t m_currentGeneration{ 1 };
        AZStd::shared_ptr<ThreadSafeData> m_threadData;
    };
} // namespace AzFramework
