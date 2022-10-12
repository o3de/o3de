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

// If not defined, compiles out all stat calls.
#define ENABLE_STAT_GATHERING

namespace Multiplayer
{
    //! @class IMultiplayerStatSystem
    //! Provides a high level stat system for Multiplayer gem and projects.
    //!
    //! Recommended use is through the following macros. Here is an example of setting up a stat group.
    //!
    //! enum { MYGROUP = 101 };
    //! DECLARE_STAT_GROUP(MYGROUP, "MyGroup");
    //!
    //! With a group defined, define a stat belonging to an existing group.
    //!
    //! enum { MYSTAT = 1001 };
    //! DECLARE_STAT(MYGROUP, MYSTAT, "MyStat");
    //!
    //! And then call SET_STAT_UINT64 to update the stat as often as needed.
    //!
    //! SET_INTEGER_STAT(MYSTAT, 1337);
    //!
    //! Stats will be written together within a group using AZ::EventLogger subsystem, which is configured using these cvars:
    //!     @cl_metricsFile, @sv_metricsFile and @bg_enableNetworkingMetrics.
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

        //! Declares a stat group with a name using a unique id.
        //! @param uniqueGroupId a unique id for a group of stats
        //! @param groupName a name for the group
        virtual void DeclareStatGroup(int uniqueGroupId, const char* groupName) = 0;

        //! Declares a stat belonging to an existing group.
        //! @param uniqueGroupId a group id already declared with  DECLARE_STAT_GROUP
        //! @param uniqueStatId a stat id already declared with DECLARE_STAT
        //! @param statName name of the stat, this does NOT take the ownership of the string
        virtual void DeclareStat(int uniqueGroupId, int uniqueStatId, const char* statName) = 0;

        //! It's recommended to use SET_INTEGER_STAT macro instead.
        //! Updates the value of a given stat already declared with DECLARE_STAT
        //! Note: metrics will take the average value of a stat within the period configured with @SetReportPeriod
        //! @param uniqueStatId a unique stat id
        //! @param value current value
        virtual void SetStat(int uniqueStatId, double value) = 0;
    };
} // namespace Multiplayer

#if defined(ENABLE_STAT_GATHERING)

#define DECLARE_STAT_GROUP(GROUPID, NAME)                                                                                                  \
    {                                                                                                                                      \
        if (auto* statSystem = AZ::Interface<IMultiplayerStatSystem>::Get())                                                               \
        {                                                                                                                                  \
            statSystem->DeclareStatGroup(GROUPID, NAME);                                                                                   \
        }                                                                                                                                  \
        else                                                                                                                               \
        {                                                                                                                                  \
            AZLOG_WARN("DECLARE_STAT_GROUP was called too early. IMultiplayerStatSystem isn't ready yet.");                                \
        }                                                                                                                                  \
    }

#define DECLARE_STAT(GROUPID, STATID, NAME)                                                                                                \
    {                                                                                                                                      \
        if (auto* statSystem = AZ::Interface<IMultiplayerStatSystem>::Get())                                                               \
        {                                                                                                                                  \
            statSystem->DeclareStat(GROUPID, STATID, NAME);                                                                            \
        }                                                                                                                                  \
        else                                                                                                                               \
        {                                                                                                                                  \
            AZLOG_WARN("DECLARE_STAT_UINT64 was called too early. IMultiplayerStatSystem isn't ready yet.");                               \
        }                                                                                                                                  \
    }

#define SET_INTEGER_STAT(STATID, VALUE)                                                                                                    \
    {                                                                                                                                      \
        if (auto* statSystem = AZ::Interface<IMultiplayerStatSystem>::Get())                                                               \
        {                                                                                                                                  \
            statSystem->SetStat(STATID, aznumeric_cast<double>(VALUE));                                                                    \
        }                                                                                                                                  \
        else                                                                                                                               \
        {                                                                                                                                  \
            AZLOG_WARN("SET_STAT_UINT64 was called too early. IMultiplayerStatSystem isn't ready yet.");                                   \
        }                                                                                                                                  \
    }

#else

#define DECLARE_STAT_GROUP()

#define DECLARE_STAT()

#define SET_INTEGER_STAT()

#endif
