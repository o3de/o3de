/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzFramework/Input/User/LocalUserId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace Achievements
{
    ////////////////////////////////////////////////////////////////////////////////////////
    // EBUS interface used to listen for achievement unlocked events
    class AchievementNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void OnAchievementUnlocked(const AZStd::string& achievementId, const AzFramework::LocalUserId& localUserId) = 0;
        virtual void OnAchievementUnlockRequested(const AZStd::string& achievementId, const AzFramework::LocalUserId& localUserId) = 0;
        virtual void OnAchievementDetailsQueried(const AzFramework::LocalUserId& localUserId, const AchievementDetails& achievementDetails) = 0;
    };
    using AchievementNotificationBus = AZ::EBus<AchievementNotifications>;
} // namespace Achievements
