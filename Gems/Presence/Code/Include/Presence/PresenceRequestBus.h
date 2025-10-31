/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Input/User/LocalUserId.h>
#include <AzCore/std/string/string.h>
#include <AzCore/EBus/EBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace Presence
{
    ///////////////////////////////////////////////////////////////////////
    // Contains presence details that can be queried through EBUS requests
    struct PresenceDetails
    {
        AZ_TYPE_INFO(PresenceDetails, "{02512959-FE0C-4FB8-A2B1-E4C675212457}");
        static void Reflect(AZ::ReflectContext* context);
        PresenceDetails();
        AzFramework::LocalUserId localUserId;
        AZ::u32 titleId;
        AZStd::string title;
        AZStd::string presence;
    };

    ////////////////////////////////////////////////////////////////////////////////////////
    // EBUS interface used for setting Presence status and retrieving current Presence setting
    class PresenceRequests
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can only be sent to and addressed by a single instance
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The parameters used for setting presence information. Different APIs have different
        //! requirements; an API will use the members it needs for the specific request.
        //! API support can be added or extended by adding required parameters here.
        struct SetPresenceParams
        {
            AZ_TYPE_INFO(SetPresenceParams, "{1AD2919C-9403-4100-A1C0-B8E642B20AB8}");

            //! Callback function to be called on main thread after successful set request
            using OnPresenceSet = AZStd::function<void(const AzFramework::LocalUserId&)>;

            //! ID of the user for whom we are setting presence
            AzFramework::LocalUserId localUserId = AzFramework::LocalUserIdNone;

            //! For presence APIs that set presence using keywords or tokens. Example: MY_PRESENCE
            AZStd::string presenceToken;

            //! For presence APIs that set presence using a plain string. Example: "My presence"
            AZStd::string presenceString;

            //! For APIs that require a language code for localization
            AZStd::string languageCode;

            OnPresenceSet OnPresenceSetCallback = nullptr;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The parameters used for requesting presence information. Different APIs have different
        //! requirements; an API will use the members it needs for the specific request.
        //! API support can be added or extended by adding required parameters here.
        struct QueryPresenceParams
        {
            AZ_TYPE_INFO(QueryPresenceParams, "{89BCF0BA-834C-4216-AAE0-B167429AA890}");

            //! Callback function to be called on main thread after successful query request
            using OnQueryPresence = AZStd::function<void(const PresenceDetails&)>;

            //! ID of the user for whom we are setting presence
            AzFramework::LocalUserId localUserId = AzFramework::LocalUserIdNone;

            //! Current presence value for the user. This is what would be visible on a user profile or to friends
            AZStd::string presence;
            OnQueryPresence OnQueryPresenceCallback = nullptr;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override will make an API request to set presence using the given SetPresenceParams
        virtual void SetPresence(const SetPresenceParams& params) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override will make an API request to query presence using the given QueryPresenceParams
        virtual void QueryPresence(const QueryPresenceParams& params) = 0;
    };
    using PresenceRequestBus = AZ::EBus<PresenceRequests>;
} // namespace Presence
