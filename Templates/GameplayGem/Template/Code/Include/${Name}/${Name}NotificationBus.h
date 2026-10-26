// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <${Name}/${SanitizedCppName}TypeIds.h>

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}Notifications
        : public AZ::EBusTraits
    {
    public:
        AZ_RTTI(${SanitizedCppName}Notifications, ${SanitizedCppName}NotificationsTypeId);

        virtual ~${SanitizedCppName}Notifications() = default;

        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnGameplayActionTriggered() {}
        virtual void OnSpeedMultiplierChanged(float /*newSpeed*/) {}
        virtual void OnStateChanged(bool /*isEnabled*/) {}
    };

    using ${SanitizedCppName}NotificationBus = AZ::EBus<${SanitizedCppName}Notifications>;
} // namespace ${SanitizedCppName}
