/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Console/ILogger.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Time/ITime.h>
#include <AzCore/base.h>

namespace Multiplayer
{
    //! @class IMultiplayerStatSystem
    //! Provides a high level stat system for Multiplayer gem and projects.
    class IMultiplayerStatSystem
    {
    public:
        AZ_RTTI(IMultiplayerStatSystem, "{B7689E92-9D5F-469D-97FA-5709BCD94DED}");

        virtual ~IMultiplayerStatSystem() = default;

        //! Initialize the system.
        virtual void Register() = 0;
        //! De-initialize the system.
        virtual void Unregister() = 0;

        //! Change how often metrics are written to AZ::EventLogger.
        //! @param period time in milliseconds between recording events
        virtual void SetReportPeriod(AZ::TimeMs period) = 0;

        //! Declares a stat group with a name using a unique id. It's recommended to use DECLARE_PERFORMANCE_STAT_GROUP macro instead.
        //! @param uniqueGroupId a unique id for a group of stats
        //! @param groupName a name for the group
        virtual void DeclareStatGroup(int uniqueGroupId, const char* groupName) = 0;

        //! Declares a stat belonging to an existing group.
        //! @param uniqueGroupId a group id already declared with DECLARE_PERFORMANCE_STAT_GROUP
        //! @param uniqueStatId a stat id already declared with DECLARE_PERFORMANCE_STAT
        //! @param statName name of the stat, this does NOT take the ownership of the string
        virtual void DeclareStat(int uniqueGroupId, int uniqueStatId, const char* statName) = 0;

        //! It's recommended to use SET_PERFORMANCE_STAT macro instead.
        //! Updates the value of a given stat already declared with DECLARE_PERFORMANCE_STAT
        //! Note: metrics will take the average value of a stat within the period configured with @SetReportPeriod
        //! @param uniqueStatId a unique stat id
        //! @param value current value
        virtual void SetStat(int uniqueStatId, double value) = 0;

        //! It's recommended to use INCREASE_PERFORMANCE_STAT macro instead.
        //! Increments the value of a given stat by one (1) that has been already declared with DECLARE_PERFORMANCE_STAT
        //! Note: metrics will take the average value of a stat within the period configured with @SetReportPeriod and reset to back to zero each time.
        //! @param uniqueStatId a unique stat id
        virtual void IncrementStat(int uniqueStatId) = 0;
    };
} // namespace Multiplayer
