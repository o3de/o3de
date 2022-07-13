/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AzToolsFramework
{
    class AssetEventNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        using BusIdType = AZ::Uuid;

        virtual void OnCreated([[maybe_unused]]const AZ::Data::AssetId& assetId) {}
    };

    using AssetEventNotificationsBus = AZ::EBus<AssetEventNotifications>;
} // namespace AzToolsFramework
