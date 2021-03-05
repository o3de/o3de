/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
    //! EBus interface used to make queries/requests related to the assignment of local user ids to
    //! local player slots, along with queries/requests related to individual local user profiles.
    //!
    //! Please note that while some platforms have no concept of a local user profile (in which case
    //! GetLocalUserProfile will always return null), most other functions will remain valid because
    //! on those platforms local user ids can be represented instead by unique input device indices.
    class LocalUserRequests : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can only be sent to and addressed by a single instance (singleton)
        ///@{
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function to get the local user id that is assigned to the primary local slot.
        //! \return The local user id that is assigned to the primary local slot, or LocalUserIdNone.
        static AzFramework::LocalUserId GetPrimaryLocalUserId();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function to get the local user id that is assigned to a specified local slot.
        //! \param[in] localPlayerSlot The local player slot to query.
        //! \return The local user id that is assigned to localPlayerSlot, or LocalUserIdNone.
        static AzFramework::LocalUserId GetLocalUserIdAt(AZ::u32 localPlayerSlot);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function to get the local user profile assigned to the primary local slot.
        //! \return A shared pointer to the local user profile if found, otherwise an empty one.
        static AZStd::shared_ptr<LocalUserProfile> GetPrimaryLocalUserProfile();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function to get a specific local user profile based on their local user id.
        //! \param[in] localUserId The local user id of the local user profile to retrieve.
        //! \return A shared pointer to the local user profile if found, otherwise an empty one.
        static AZStd::shared_ptr<LocalUserProfile> GetLocalUserProfile(AzFramework::LocalUserId localUserId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function to get the specific local user profile assigned to a local slot.
        //! \param[in] localPlayerSlot The local player slot to query.
        //! \return A shared pointer to the local user profile if found, otherwise an empty one.
        static AZStd::shared_ptr<LocalUserProfile> GetLocalUserProfileAt(AZ::u32 localPlayerSlot);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Finds a specific local user profile based on their local user id.
        //! \param[in] localUserId The local user id of the local user profile to retrieve.
        //! \return A shared pointer to the local user profile if found, otherwise an empty one.
        virtual AZStd::shared_ptr<LocalUserProfile> FindLocalUserProfile(AzFramework::LocalUserId localUserId) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Query the maximum number of local uses that can be signed into the system concurrently.
        //! \return The maximum number of local uses that can be signed into the system concurrently.
        virtual AZ::u32 GetMaxLocalUsers() const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Query whether a local user id is signed in. Please note that a user can be assigned to a
        //! local player slot but signed out, or signed in but not assigned to a local player slot.
        //!
        //! On platforms with no concept of a local user profile, local user ids are instead unique
        //! input device indices, so while it may seem counter-intuitive this may still return true.
        //!
        //! \param[in] localUserId The local user id to query.
        //! \return True if localUserId is signed in, false otherwise.
        virtual bool IsLocalUserSignedIn(AzFramework::LocalUserId localUserId) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the user name associated with a local user id. Platforms that have no concept of a
        //! local user profile will return "Player N" where "N" is the local player slot currently
        //! occupied by localUserId, or an empty string if they don't currently occupy a local slot.
        //! \param[in] localUserId The local user id to query.
        //! \return The user name that is associated with localUserId.
        virtual AZStd::string GetLocalUserName(AzFramework::LocalUserId localUserId) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Assign a local user id into a local player slot.
        //! \param[in] localUserId The local user id to assign into a local player slot.
        //! \param[in] localPlayerSlot The local player slot to assign the local user id into.
        //!                            LocalPlayerSlotAny uses the first available local slot.
        //! \return The local player slot that localUserId was assigned to, or LocalPlayerSlotNone.
        virtual AZ::u32 AssignLocalUserIdToLocalPlayerSlot(AzFramework::LocalUserId localUserId,
                                                           AZ::u32 localPlayerSlot = LocalPlayerSlotAny) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Remove a local user id from a local player slot.
        //! \param[in] localUserId The local user id to remove from a local player slot.
        //! \return The local player slot that localUserId was removed from, or LocalPlayerSlotNone.
        virtual AZ::u32 RemoveLocalUserIdFromLocalPlayerSlot(AzFramework::LocalUserId localUserId) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the local user id that is assigned to a local player slot.
        //! \param[in] localPlayerSlot The local player slot query.
        //! \return The local user id that is assigned to localPlayerSlot, or LocalUserIdNone.
        virtual AzFramework::LocalUserId GetLocalUserIdAssignedToLocalPlayerSlot(AZ::u32 localPlayerSlot) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the local player slot that a local user id is assigned to.
        //! \param[in] localUserId The local user id to query.
        //! \return The local player slot that has localUserId assigned, or LocalPlayerSlotNone.
        virtual AZ::u32 GetLocalPlayerSlotOccupiedByLocalUserId(AzFramework::LocalUserId localUserId) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Clears all previously assigned local user id to local player slot associations.
        virtual void ClearAllLocalUserIdToLocalPlayerSlotAssignments() = 0;
    };
    using LocalUserRequestBus = AZ::EBus<LocalUserRequests>;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline AzFramework::LocalUserId LocalUserRequests::GetPrimaryLocalUserId()
    {
        return GetLocalUserIdAt(LocalPlayerSlotPrimary);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline AzFramework::LocalUserId LocalUserRequests::GetLocalUserIdAt(AZ::u32 localPlayerSlot)
    {
        AzFramework::LocalUserId localUserId = AzFramework::LocalUserIdNone;
        LocalUserRequestBus::BroadcastResult(localUserId,
                                             &LocalUserRequests::GetLocalUserIdAssignedToLocalPlayerSlot,
                                             localPlayerSlot);
        return localUserId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline AZStd::shared_ptr<LocalUserProfile> LocalUserRequests::GetPrimaryLocalUserProfile()
    {
        return GetLocalUserProfileAt(LocalPlayerSlotPrimary);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline AZStd::shared_ptr<LocalUserProfile> LocalUserRequests::GetLocalUserProfile(AzFramework::LocalUserId localUserId)
    {
        AZStd::shared_ptr<LocalUserProfile> localUserProfile;
        LocalUserRequestBus::BroadcastResult(localUserProfile,
                                             &LocalUserRequests::FindLocalUserProfile,
                                             localUserId);
        return localUserProfile;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline AZStd::shared_ptr<LocalUserProfile> LocalUserRequests::GetLocalUserProfileAt(AZ::u32 localPlayerSlot)
    {
        const AzFramework::LocalUserId localUserId = GetLocalUserIdAt(localPlayerSlot);
        return localUserId != AzFramework::LocalUserIdNone ? GetLocalUserProfile(localUserId) : nullptr;
    }
} // namespace LocalUser
