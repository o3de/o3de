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

namespace Achievements
{
    ////////////////////////////////////////////////////////////////////////////////////////
    // Contains achievements details that can be queried through EBUS requests and will contain
    // achievement name, description, value, and query result
    struct AchievementDetails
    {
        AZ_TYPE_INFO(AchievementDetails, "{3310A37C-4B91-4529-B893-38C89AD69F82}");
        static void Reflect(AZ::ReflectContext* context);
        AchievementDetails();
        AZ::u32             id;                   // achievement Id
        AZStd::string       name;                 // name of the achievement
        AZStd::string       desc;                 // achievement description
        int                 rewardValue;          // "gamerscore" value of the achievement
        AZ::u32             currentProgress;      // current progress towards unlock requirement
        bool                unlocked;             // whether or not achievement is unlocked
        bool                secret;               // is achievement secret or hidden
    };


    ////////////////////////////////////////////////////////////////////////////////////////
    // EBUS interface used to make requests for achievement details, unlock status, and unlocking
    class AchievementRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        struct UnlockAchievementParams
        {
            AZ_TYPE_INFO(UnlockAchievementParams, "{9D28BB0F-2531-4371-9A91-1EE9226F2DE7}");
            using OnAchievementUnlocked = AZStd::function<void(const AzFramework::LocalUserId&, const AZStd::string&)>;
            AzFramework::LocalUserId localUserId = AzFramework::LocalUserIdNone;
            AZStd::string achievementId;
            AZ::u32 percentage = 0;
            OnAchievementUnlocked OnAchievementUnlockCallback = nullptr;
        };

        struct QueryAchievementParams
        {
            AZ_TYPE_INFO(QueryAchievementParams, "{04195FE5-DBA9-45DE-BDB2-C2EC2D523BB5}");
            using OnAchievementDetailsQueried = AZStd::function<void(const AzFramework::LocalUserId&, const AchievementDetails&)>;
            AzFramework::LocalUserId localUserId = AzFramework::LocalUserIdNone;
            AZStd::string achievementId;
            OnAchievementDetailsQueried OnAchievementDetailsQueriedCallback = nullptr;
        };

        ////////////////////////////////////////////////////////////////////////////////////////
        // Unlocks the given achievement for the current player or adds towards progression to unlocking
        virtual void UnlockAchievement(const UnlockAchievementParams& params) = 0;
        ////////////////////////////////////////////////////////////////////////////////////////
        // Queries details of an achievements. fills in a structure with the name, description
        // and reward value
        virtual void QueryAchievementDetails(const QueryAchievementParams& params) = 0;
    };
    using AchievementRequestBus = AZ::EBus<AchievementRequests>;
} // namespace Achievements
