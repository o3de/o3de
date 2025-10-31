/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

namespace AzToolsFramework
{
    class EntityHighlightMessages
        : public AZ::EBusTraits
    {
    public:

        using Bus = AZ::EBus<EntityHighlightMessages>;

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // there is one bus that everyone on it listens to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // every listener registers unordered on this bus

        virtual void EntityHighlightRequested(AZ::EntityId) = 0;
        virtual void EntityStrongHighlightRequested(AZ::EntityId) = 0;
    };
}

AZ_DECLARE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::EntityHighlightMessages);
