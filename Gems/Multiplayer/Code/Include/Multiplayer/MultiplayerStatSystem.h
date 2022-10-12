/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Time/ITime.h>

// If not defined, compiles out all stat calls.
#define ENABLE_STAT_GATHERING

namespace Multiplayer
{
    //! @class IMultiplayerStatSystem
    //! Provides a high level stat system for Multiplayer systems.
    //!
    //! Recommend use is through using macros. Here is an example of setting up a group and a stat once.
    //!
    //! enum { MYGROUP = 123 };
    //! DECLARE_STAT_GROUP(MYGROUP, "MyGroup");
    //!
    //! enum { MYSTAT = 1001 };
    //! DECLARE_STAT_UINT64(MYGROUP, MYSTAT, "MyStat");
    //!
    //! And then call SET_STAT_UINT64 to update the stat as often as needed.
    //!
    //! SET_STAT_UINT64(MYSTAT, 1337);
    //!
    //! Stats will be written down the AZ::EventLogger, which is configured using these cvars:
    //!     @cl_metricsFile, @sv_metricsFile and @bg_enableNetworkingMetrics.
    class IMultiplayerStatSystem
    {
    public:
        AZ_RTTI(IMultiplayerStatSystem, "{B7689E92-9D5F-469D-97FA-5709BCD94DED}");

        virtual ~IMultiplayerStatSystem() = default;

        //! Initialize stat system.
        virtual void Register() = 0;
        //! De-initialize stat system.
        virtual void Unregister() = 0;
        //! Sets how often metrics are written to AZ::EventLogger.
        virtual void SetReportPeriod(AZ::TimeMs period) = 0;

        //! Declares a stat group with a name using a unique id.
        virtual void DeclareStatGroup(int uniqueGroupId, const char* groupName) = 0;
        //! Declares a stat belonging to an existing group.
        virtual void DeclareStatTypeIntU64(int uniqueGroupId, int uniqueStatId, const char* statName) = 0;

        //! Overwrites the counter value.
        virtual void SetStatTypeIntU64(int uniqueStatId, AZ::u64 value) = 0;
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

#define DECLARE_STAT_UINT64(GROUPID, STATID, NAME)                                                                                         \
    {                                                                                                                                      \
        if (auto* statSystem = AZ::Interface<IMultiplayerStatSystem>::Get())                                                               \
        {                                                                                                                                  \
            statSystem->DeclareStatTypeIntU64(GROUPID, STATID, NAME);                                                                      \
        }                                                                                                                                  \
        else                                                                                                                               \
        {                                                                                                                                  \
            AZLOG_WARN("DECLARE_STAT_UINT64 was called too early. IMultiplayerStatSystem isn't ready yet.");                               \
        }                                                                                                                                  \
    }

#define SET_STAT_UINT64(STATID, VALUE)                                                                                                     \
    {                                                                                                                                      \
        if (auto* statSystem = AZ::Interface<IMultiplayerStatSystem>::Get())                                                               \
        {                                                                                                                                  \
            statSystem->SetStatTypeIntU64(STATID, VALUE);                                                                                  \
        }                                                                                                                                  \
        else                                                                                                                               \
        {                                                                                                                                  \
            AZLOG_WARN("SET_STAT_UINT64 was called too early. IMultiplayerStatSystem isn't ready yet.");                                   \
        }                                                                                                                                  \
    }

#else

#define DECLARE_STAT_GROUP()

#define DECLARE_STAT_UINT64()

#define SET_STAT_UINT64()

#endif
