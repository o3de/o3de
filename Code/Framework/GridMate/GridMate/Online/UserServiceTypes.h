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
#ifndef GM_USER_SERVICE_TYPES_H
#define GM_USER_SERVICE_TYPES_H

#include <GridMate/Types.h>
#include <GridMate/String/string.h>

namespace GridMate
{
    /**
     * User signin state
     */
    enum OLSSigninState
    {
        OLS_SigninUnknown,
        OLS_NotSignedIn,        // There is no user signed in
        OLS_SignedInOffline,    // User signed in without online capabilities
        OLS_SignedInOnline,     // User signed in with online capabilities
        OLS_SigningOut,         // User is in the process of signing out
    };

    /**
     * service network state
     */
    enum OLSOnlineState
    {
        OLS_OnlineUnknown,
        OLS_NoNetwork,          // No NIC or network is unplugged
        OLS_Offline,            // No online access
        OLS_Online,             // Has online access
    };

    /**
     * Supported privilege types
     */
    enum OLSUserPrivilege
    {
        OLS_UserPrivilegeMP,
        OLS_UserPrivilegeRecordDVR,
        OLS_UserPrivilegePurchaseContent,
        OLS_UserPrivilegeVoiceChat,
        OLS_UserPrivilegeLeaderboards
    };

    /**
     * Base class for platform dependent player id.
     */
    struct PlayerId
    {
        PlayerId(ServiceType serviceType)
            : m_serviceType(serviceType)  {}
        virtual ~PlayerId()                                             {}

        // Compare 2 PlayerId IDs
        virtual bool Compare(const PlayerId& userId) const = 0;

        // Returns a printable string representation of the id.
        virtual gridmate_string ToString() const = 0;

        ServiceType GetType() const { return m_serviceType; }

    protected:
        ServiceType m_serviceType;
    };

    /**
     * Interface class for a local player/member.
     */
    class ILocalMember
    {
    public:
        virtual ~ILocalMember() {}
        // SignIn
        virtual OLSSigninState  GetSigninState() const = 0;

        virtual const PlayerId* GetPlayerId() const = 0;

        // Pad number / info    ???
        virtual unsigned int    GetControllerIndex() const = 0;
        virtual const char*     GetName() const = 0;
        virtual bool            IsGuest() const = 0;

        // Friends List
        virtual void            RefreshFriends() = 0;
        virtual bool            IsFriendsListRefreshing() const = 0;
        virtual unsigned int    GetFriendsCount() const = 0;
        virtual const char*     GetFriendName(unsigned int idx) const = 0;
        virtual const PlayerId* GetFriendPlayerId(unsigned int idx) const = 0;
        virtual OLSSigninState  GetFriendSigninState(unsigned int idx) const = 0;
        virtual bool            IsFriendPlayingTitle(unsigned int idx) const = 0;
        virtual const char*     GetFriendPresenceDetails(unsigned int idx) const = 0;
        virtual bool            IsFriendsWith(const PlayerId* playerId) const = 0;
    };

    /**
    * Generic invite structure
    * pPlatformSpecific contains the native structure used by each platform
    */
    struct  InviteInfo
    {
        InviteInfo()
            : m_localMember(nullptr) {}

        ILocalMember* m_localMember;
    };
}   // namespace GridMate

#endif  // GM_USER_SERVICE_TYPES_H
#pragma once
