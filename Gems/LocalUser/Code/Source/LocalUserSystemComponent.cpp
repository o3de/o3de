/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LocalUserSystemComponent.h>

#include <LocalUser/LocalUserNotificationBus.h>

#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/std/string/conversions.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace LocalUser
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class LocalUserNotificationBusBehaviorHandler
        : public LocalUserNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        AZ_EBUS_BEHAVIOR_BINDER(LocalUserNotificationBusBehaviorHandler, "{6A3B1CAB-92BE-4773-A3AE-470203D70662}", AZ::SystemAllocator
            , OnLocalUserSignedIn
            , OnLocalUserSignedOut
            , OnLocalUserIdAssignedToLocalPlayerSlot
            , OnLocalUserIdRemovedFromLocalPlayerSlot
        );

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnLocalUserSignedIn(AzFramework::LocalUserId localUserId) override
        {
            Call(FN_OnLocalUserSignedIn, localUserId);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnLocalUserSignedOut(AzFramework::LocalUserId localUserId) override
        {
            Call(FN_OnLocalUserSignedOut, localUserId);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnLocalUserIdAssignedToLocalPlayerSlot(AzFramework::LocalUserId localUserId,
                                                    AZ::u32 newLocalPlayerSlot,
                                                    AZ::u32 previousLocalPlayerSlot) override
        {
            Call(FN_OnLocalUserIdAssignedToLocalPlayerSlot, localUserId, newLocalPlayerSlot, previousLocalPlayerSlot);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void OnLocalUserIdRemovedFromLocalPlayerSlot(AzFramework::LocalUserId localUserId,
                                                     AZ::u32 localPlayerSlot) override
        {
            Call(FN_OnLocalUserIdRemovedFromLocalPlayerSlot, localUserId, localPlayerSlot);
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void LocalUserSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LocalUserSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<LocalUserSystemComponent>("LocalUser", "Provides functionality for mapping local user ids to local player slots and managing local user profiles.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<LocalUserNotificationBus>("LocalUserNotificationBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Category, "LocalUser")
                ->Handler<LocalUserNotificationBusBehaviorHandler>()
            ;

            behaviorContext->EBus<LocalUserRequestBus>("LocalUserRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Category, "LocalUser")
                ->Event("GetMaxLocalUsers", &LocalUserRequestBus::Events::GetMaxLocalUsers)
                ->Event("IsLocalUserSignedIn", &LocalUserRequestBus::Events::IsLocalUserSignedIn)
                ->Event("GetLocalUserName", &LocalUserRequestBus::Events::GetLocalUserName)
                ->Event("AssignLocalUserIdToLocalPlayerSlot", &LocalUserRequestBus::Events::AssignLocalUserIdToLocalPlayerSlot)
                ->Event("RemoveLocalUserIdFromLocalPlayerSlot", &LocalUserRequestBus::Events::RemoveLocalUserIdFromLocalPlayerSlot)
                ->Event("GetLocalUserIdAssignedToLocalPlayerSlot", &LocalUserRequestBus::Events::GetLocalUserIdAssignedToLocalPlayerSlot)
                ->Event("GetLocalPlayerSlotOccupiedByLocalUserId", &LocalUserRequestBus::Events::GetLocalPlayerSlotOccupiedByLocalUserId)
                ->Event("ClearAllLocalUserIdToLocalPlayerSlotAssignments", &LocalUserRequestBus::Events::ClearAllLocalUserIdToLocalPlayerSlotAssignments)
            ;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void LocalUserSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("LocalUserService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void LocalUserSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("LocalUserService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    LocalUserSystemComponent::LocalUserSystemComponent()
    {
        for (AZ::u32 i = 0; i < LocalPlayerSlotMax; ++i)
        {
            m_localUserIdsByLocalPlayerSlot[i] = AzFramework::LocalUserIdNone;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void LocalUserSystemComponent::Activate()
    {
        m_pimpl.reset(Implementation::Create());
        LocalUserRequestBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void LocalUserSystemComponent::Deactivate()
    {
        ClearAllLocalUserIdToLocalPlayerSlotAssignments();
        LocalUserRequestBus::Handler::BusDisconnect();
        m_pimpl.reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::shared_ptr<LocalUserProfile> LocalUserSystemComponent::FindLocalUserProfile(AzFramework::LocalUserId localUserId)
    {
        return m_pimpl ? m_pimpl->FindLocalUserProfile(localUserId) : nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::u32 LocalUserSystemComponent::GetMaxLocalUsers() const
    {
        if (m_pimpl)
        {
            return m_pimpl->GetMaxLocalUsers();
        }

        // On platforms with no concept of a local user profile the local user id corresponds
        // to a unique input device index, so the maximum is the number of supported gamepads.
        auto deviceGamepadImplFactory = AZ::Interface<AzFramework::InputDeviceGamepad::ImplementationFactory>::Get();
        return (deviceGamepadImplFactory != nullptr) ? deviceGamepadImplFactory->GetMaxSupportedGamepads() : 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool LocalUserSystemComponent::IsLocalUserSignedIn(AzFramework::LocalUserId localUserId)
    {
        if (m_pimpl)
        {
            return m_pimpl->IsLocalUserSignedIn(localUserId);
        }

        // On platforms with no concept of a local user profile the local user id corresponds
        // to a unique input device index, and is therefore always considered to be signed in.
        return localUserId != AzFramework::LocalUserIdNone;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string LocalUserSystemComponent::GetLocalUserName(AzFramework::LocalUserId localUserId)
    {
        if (m_pimpl)
        {
            return m_pimpl->GetLocalUserName(localUserId);
        }

        // On platforms that have no concept of a local user profile, return "Player N" where "N" is
        // the local player slot currently occupied by localUserId, otherwise return an empty string.
        const AZ::u32 localPlayerSlot = GetLocalPlayerSlotOccupiedByLocalUserId(localUserId);
        return (localPlayerSlot < LocalPlayerSlotMax) ?
                AZStd::string("Player ") + AZStd::to_string(localPlayerSlot + 1) :
                "";
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::u32 LocalUserSystemComponent::AssignLocalUserIdToLocalPlayerSlot(AzFramework::LocalUserId localUserId,
                                                                         AZ::u32 localPlayerSlot)
    {
        AZ_Warning("LocalUserSystemComponent",
                   localUserId != AzFramework::LocalUserIdAny,
                   "Assigning LocalUserIdAny to local player slot %d.\n"
                   "You should likely prompt the user to sign-in first,\n"
                   "probably by using InputDevice::PromptLocalUserSignIn\n");

        const AZ::u32 existingLocalPlayerSlot = GetLocalPlayerSlotOccupiedByLocalUserId(localUserId);
        if (localPlayerSlot < LocalPlayerSlotMax)
        {
            // A specific slot has been requested...
            if (m_localUserIdsByLocalPlayerSlot[localPlayerSlot] == AzFramework::LocalUserIdNone)
            {
                // ...and it is unoccupied, so assign the user into the slot
                // and remove the user from any existing slot it occupied.
                m_localUserIdsByLocalPlayerSlot[localPlayerSlot] = localUserId;
                if (existingLocalPlayerSlot < LocalPlayerSlotMax)
                {
                    m_localUserIdsByLocalPlayerSlot[existingLocalPlayerSlot] = AzFramework::LocalUserIdNone;
                }
                LocalUserNotificationBus::Broadcast(&LocalUserNotifications::OnLocalUserIdAssignedToLocalPlayerSlot,
                                                    localUserId,
                                                    localPlayerSlot,
                                                    existingLocalPlayerSlot);
                return localPlayerSlot;
            }
            else
            {
                // ...and it is occupied, so just return the existing slot
                // that the user occupies, which may be LocalPlayerSlotNone.
                return existingLocalPlayerSlot;
            }
        }

        if (existingLocalPlayerSlot < LocalPlayerSlotMax)
        {
            // The user is already assigned to a slot and the requested
            // slot is already occupied (or any slot was requested).
            return existingLocalPlayerSlot;
        }

        if (localPlayerSlot == LocalPlayerSlotAny)
        {
            // The user is not already assigned to a slot, and any slot
            // was requested, so assign the user to the first empty slot.
            for (AZ::u32 i = 0; i < LocalPlayerSlotMax; ++i)
            {
                if (m_localUserIdsByLocalPlayerSlot[i] == AzFramework::LocalUserIdNone)
                {
                    m_localUserIdsByLocalPlayerSlot[i] = localUserId;
                    LocalUserNotificationBus::Broadcast(&LocalUserNotifications::OnLocalUserIdAssignedToLocalPlayerSlot,
                                                        localUserId,
                                                        i,
                                                        LocalPlayerSlotNone);
                    return i;
                }
            }
        }

        // Unable to assign the local user id to the requested local player slot
        return LocalPlayerSlotNone;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::u32 LocalUserSystemComponent::RemoveLocalUserIdFromLocalPlayerSlot(AzFramework::LocalUserId localUserId)
    {
        const AZ::u32 existingLocalPlayerSlot = GetLocalPlayerSlotOccupiedByLocalUserId(localUserId);
        if (existingLocalPlayerSlot < LocalPlayerSlotMax)
        {
            m_localUserIdsByLocalPlayerSlot[existingLocalPlayerSlot] = AzFramework::LocalUserIdNone;
            LocalUserNotificationBus::Broadcast(&LocalUserNotifications::OnLocalUserIdRemovedFromLocalPlayerSlot,
                                                localUserId,
                                                existingLocalPlayerSlot);
        }
        return existingLocalPlayerSlot;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AzFramework::LocalUserId LocalUserSystemComponent::GetLocalUserIdAssignedToLocalPlayerSlot(AZ::u32 localPlayerSlot)
    {
        return localPlayerSlot < LocalPlayerSlotMax ?
               m_localUserIdsByLocalPlayerSlot[localPlayerSlot] :
               AzFramework::LocalUserIdNone;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::u32 LocalUserSystemComponent::GetLocalPlayerSlotOccupiedByLocalUserId(AzFramework::LocalUserId localUserId)
    {
        for (AZ::u32 i = 0; i < LocalPlayerSlotMax; ++i)
        {
            if (m_localUserIdsByLocalPlayerSlot[i] == localUserId)
            {
                return i;
            }
        }
        return LocalPlayerSlotNone;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void LocalUserSystemComponent::ClearAllLocalUserIdToLocalPlayerSlotAssignments()
    {
        for (AZ::u32 i = 0; i < LocalPlayerSlotMax; ++i)
        {
            const AzFramework::LocalUserId& localUserId = m_localUserIdsByLocalPlayerSlot[i];
            if (localUserId != AzFramework::LocalUserIdNone)
            {
                RemoveLocalUserIdFromLocalPlayerSlot(localUserId);
            }
        }
    }
} // namespace LocalUser
