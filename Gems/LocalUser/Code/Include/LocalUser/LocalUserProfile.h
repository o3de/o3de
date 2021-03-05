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

#include <AzFramework/Input/User/LocalUserId.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace LocalUser
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Abstract class that represents a local user profile, uniquely identified by a local user id.
    //! Please note that on some platforms there does not exist any concept of a local user profile.
    class LocalUserProfile
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(LocalUserProfile, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(LocalUserProfile, "{2A681065-FA77-46CF-9533-04D6C5C5EAF0}");

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] localUserId The local user id that uniquely identifies the local user profile
        explicit LocalUserProfile(AzFramework::LocalUserId locaUserId) : m_locaUserId(locaUserId) {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~LocalUserProfile() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the local user id that uniquely identifies this local user profile on the system.
        //! \return The local user id that uniquely identifies this local user profile on the system.
        inline AzFramework::LocalUserId GetLocalUserId() const { return m_locaUserId; }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Query whether this local user profile is currently signed in.
        //! \return True if this local user profile is currently signed in, false otherwise.
        virtual bool IsSignedIn() const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Query the user name associated with this local user profile.
        //! \return A UTF-8 string containing this local user's user name.
        virtual AZStd::string GetUserName() const = 0;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The local user id that uniquely identifies this local user profile.
        const AzFramework::LocalUserId m_locaUserId = AzFramework::LocalUserIdNone;
    };
} // namespace LocalUser
