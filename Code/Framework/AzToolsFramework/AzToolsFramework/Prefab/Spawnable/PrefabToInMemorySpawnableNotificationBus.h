/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzFramework/Entity/EntityContext.h>

namespace AzToolsFramework::Prefab
{
    //! This EBus is used to send notifications during the conversion of a prefab template to an in-memory spawnable
    class PrefabToInMemorySpawnableNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Notification to allow systems to access a spawnable before it's been modified, for example, by aliasing
        //! @param spawnable the spawnable that is being prepared
        //! @param assetHint the spawnable asset name
        virtual void OnPreparingInMemorySpawnableFromPrefab(
            [[maybe_unused]] const AzFramework::Spawnable& spawnable, [[maybe_unused]] const AZStd::string& assetHint) {}

    protected:
        ~PrefabToInMemorySpawnableNotifications() = default;
    };

    using PrefabToInMemorySpawnableNotificationBus = AZ::EBus<PrefabToInMemorySpawnableNotifications>;

} // namespace AzToolsFramework::Prefab
