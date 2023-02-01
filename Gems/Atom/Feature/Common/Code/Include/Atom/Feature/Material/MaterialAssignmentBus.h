/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AZ
{
    namespace Render
    {
        //! Components should inherit from the MaterialAssignmentNotificationBus to receive notifications
        class MaterialAssignmentNotifications
            : public AZ::EBusTraits
        {
        public :
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
            using BusIdType = AZ::Data::AssetId;
            using MutexType = AZStd::mutex;

            virtual void OnRebuildMaterialInstance() = 0;
        };
        using MaterialAssignmentNotificationBus = AZ::EBus<MaterialAssignmentNotifications>;
    } // namespace Render
} // namespace AZ
