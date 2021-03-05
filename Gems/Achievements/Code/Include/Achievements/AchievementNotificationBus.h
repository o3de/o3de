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
