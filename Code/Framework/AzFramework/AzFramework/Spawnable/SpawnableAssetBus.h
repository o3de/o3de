/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzFramework/Spawnable/Spawnable.h>

namespace AzFramework
{
    class SpawnableAssetEvents : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;

        //! Callback to allow the entity aliases in a spawnable to adjusted based on runtime requirements.
        //! This will be called by the Asset Manager as part of the creation of the spawnable asset from loaded file data. Any work done
        //! in this callback will be counted towards the maximum amount of time allocated to asset handlers to construct their assets,
        //! it's recommended to keep work done in this callback to a minimum and prefer delaying any complex processing.
        //!
        //! ALERT: Do not start blocking asset requests in this callback.
        //!     Since this is part of the Asset Manager's asset streaming, doing a blocking load in this callback will cause the job
        //!     processing the spawnable asset to locked out of doing any asset streaming work. If there are more spawnables doing
        //!     this than there are job threads available the engine will enter a deadlock situation as no more assets can complete
        //!     loading and no job threads become free as they're all waiting for assets to complete. It is however safe to queue
        //!     an asset for loading.
        virtual void OnResolveAliases(
            Spawnable::EntityAliasVisitor& aliases, const SpawnableMetaData& metadata, const Spawnable::EntityList& entities) = 0;
    };

    using SpawnableAssetEventsBus = AZ::EBus<SpawnableAssetEvents>;
} // namespace AzFramework
