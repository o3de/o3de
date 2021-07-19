/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LocalUser/LocalPlayerSlot.h>
#include <LocalUser/LocalUserProfile.h>

#include <AzCore/EBus/EBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace LocalUser
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to listen for notifications related to assignment of local user ids to
    //! local player slots in addition to notifications related to individual local user profiles.
    class LocalUserNotifications : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: local user notifications are addressed to a single address
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: local user notifications can be handled by multiple listeners
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when a local user signs into the system
        //! \param[in] localUserId The local user id that signed into the system
        virtual void OnLocalUserSignedIn([[maybe_unused]] AzFramework::LocalUserId localUserId) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when a local user signs out of the system
        //! \param[in] localUserId The local user id that signed out of the system
        virtual void OnLocalUserSignedOut([[maybe_unused]] AzFramework::LocalUserId localUserId) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when a local user id is assigned to a local player slot
        //! \param[in] localUserId The local user id that was assigned to a local player slot.
        //! \param[in] newLocalPlayerSlot The local player slot that the local user id now occupies.
        //! \param[in] previousLocalPlayerSlot The local player slot that the local user id previously occupied, or LocalPlayerSlotNone.
        virtual void OnLocalUserIdAssignedToLocalPlayerSlot([[maybe_unused]] AzFramework::LocalUserId localUserId,
                                                            [[maybe_unused]] AZ::u32 newLocalPlayerSlot,
                                                            [[maybe_unused]] AZ::u32 previousLocalPlayerSlot = LocalPlayerSlotNone) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when a local user id is removed from a local player slot
        //! \param[in] localUserId The local user id that was removed from a local player slot.
        //! \param[in] localPlayerSlot The local player slot that the local user id was removed from.
        virtual void OnLocalUserIdRemovedFromLocalPlayerSlot([[maybe_unused]] AzFramework::LocalUserId localUserId,
                                                             [[maybe_unused]] AZ::u32 localPlayerSlot) {}
    };
    using LocalUserNotificationBus = AZ::EBus<LocalUserNotifications>;
} // namespace LocalUser
