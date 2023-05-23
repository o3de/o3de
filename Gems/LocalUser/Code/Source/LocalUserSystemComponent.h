/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LocalUser/LocalUserRequestBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace LocalUser
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! A system component providing functionality for mapping local user ids to local player slots,
    //! and managing local user profiles. Please note that while some platforms have no concept of a
    //! local user profile, the functionality for assigning local user ids to local player slots can
    //! still be used because local user ids are represented instead by unique input device indices.
    class LocalUserSystemComponent : public AZ::Component
                                   , public LocalUserRequestBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::Component Setup
        AZ_COMPONENT(LocalUserSystemComponent, "{D22DBCC8-9F44-47F6-86CA-0BE1F52D1727}");

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::ComponentDescriptor::Reflect
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::ComponentDescriptor::GetProvidedServices
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::ComponentDescriptor::GetIncompatibleServices
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        LocalUserSystemComponent();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~LocalUserSystemComponent() override = default;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::Component::Activate
        void Activate() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::Component::Deactivate
        void Deactivate() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref LocalUser::LocalUserRequests::FindLocalUserProfile
        AZStd::shared_ptr<LocalUserProfile> FindLocalUserProfile(AzFramework::LocalUserId localUserId) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref LocalUser::LocalUserRequests::GetMaxLocalUsers
        AZ::u32 GetMaxLocalUsers() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref LocalUser::LocalUserRequests::IsLocalUserSignedIn
        bool IsLocalUserSignedIn(AzFramework::LocalUserId localUserId) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref LocalUser::LocalUserRequests::GetLocalUserName
        AZStd::string GetLocalUserName(AzFramework::LocalUserId localUserId) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot
        AZ::u32 AssignLocalUserIdToLocalPlayerSlot(AzFramework::LocalUserId localUserId,
                                                   AZ::u32 localPlayerSlot = LocalPlayerSlotAny) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref LocalUser::LocalUserRequests::RemoveLocalUserIdFromLocalPlayerSlot
        AZ::u32 RemoveLocalUserIdFromLocalPlayerSlot(AzFramework::LocalUserId localUserId) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref LocalUser::LocalUserRequests::GetLocalUserIdAssignedToLocalPlayerSlot
        AzFramework::LocalUserId GetLocalUserIdAssignedToLocalPlayerSlot(AZ::u32 localPlayerSlot) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref LocalUser::LocalUserRequests::GetLocalPlayerSlotOccupiedByLocalUserId
        AZ::u32 GetLocalPlayerSlotOccupiedByLocalUserId(AzFramework::LocalUserId localUserId) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref LocalUser::LocalUserRequests::ClearAllLocalUserIdToLocalPlayerSlotAssignments
        void ClearAllLocalUserIdToLocalPlayerSlotAssignments() override;

    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Base class for platform specific implementations of the local user system component
        class Implementation
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Allocator
            AZ_CLASS_ALLOCATOR(Implementation, AZ::SystemAllocator);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Default factory create function
            //! \param[in] localUserSystemComponent Reference to the parent being implemented
            static Implementation* Create();

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            //! \param[in] localUserSystemComponent Reference to the parent being implemented
            Implementation() {}

            ////////////////////////////////////////////////////////////////////////////////////////
            // Disable copying
            AZ_DISABLE_COPY_MOVE(Implementation);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Default destructor
            virtual ~Implementation() = default;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Finds a specific local user profile based on their local user id.
            //! \param[in] localUserId The local user id of the local user profile to retrieve.
            //! \return A shared pointer to the local user profile if found, otherwise an empty one.
            virtual AZStd::shared_ptr<LocalUserProfile> FindLocalUserProfile(AzFramework::LocalUserId localUserId) = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Query the maximum number of local uses that can be signed in concurrently.
            //! \return The maximum number of local uses that can be signed in concurrently.
            virtual AZ::u32 GetMaxLocalUsers() const = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Query whether a local user id is signed in.
            //! \param[in] localUserId The local user id to query.
            //! \return True if localUserId is signed in, false otherwise.
            virtual bool IsLocalUserSignedIn(AzFramework::LocalUserId localUserId) = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Get the user name associated with a local user id.
            //! \param[in] localUserId The local user id to query.
            //! \return The user name that is associated with localUserId.
            virtual AZStd::string GetLocalUserName(AzFramework::LocalUserId localUserId) = 0;
        };

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Private pointer to the platform specific implementation
        AZStd::unique_ptr<Implementation> m_pimpl;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! An array of local user ids indexed by their assigned local player slot
        AzFramework::LocalUserId m_localUserIdsByLocalPlayerSlot[LocalPlayerSlotMax];
    };
} // namespace LocalUser
