/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/any.h>
#include <AzCore/std/string/string.h>

namespace EMStudio
{
    class PluginOptionsNotifications 
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZStd::string;
        //////////////////////////////////////////////////////////////////////////

        virtual ~PluginOptionsNotifications() {}

        virtual void OnOptionChanged([[maybe_unused]] const AZStd::string& optionChanged) {}
    };

    using PluginOptionsNotificationsBus = AZ::EBus<PluginOptionsNotifications>;
}
