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
 // DISABLE_MULTIPLAYER_STATS can be defined in a cmake target to compile out the stat calls.
#ifndef DISABLE_MULTIPLAYER_STATS
#define ENABLE_PERFORMANCE_STAT_GATHERING
#endif


//! Provides a high level stat system for Multiplayer gem and projects.
//!
//! Recommended use is through the following macros. Here is an example of setting up a stat group.
//!
//! enum { MYGROUP = 101 };
//! DECLARE_PERFORMANCE_STAT_GROUP(MYGROUP, "MyGroup");
//!
//! With a group defined, define a stat belonging to an existing group.
//!
//! enum { MYSTAT = 1001 };
//! DECLARE_PERFORMANCE_STAT(MYGROUP, MYSTAT, "MyStat");
//!
//! And then call SET_PERFORMANCE_STAT to update the stat as often as needed.
//!
//! SET_PERFORMANCE_STAT(MYSTAT, 1337);
//!
//! Stats will be written together within a group using AZ::EventLogger subsystem, which is configured using these cvars:
//!     @cl_metricsFile, @sv_metricsFile and @bg_enableNetworkingMetrics.

#if defined(ENABLE_PERFORMANCE_STAT_GATHERING)
#include <Multiplayer/MultiplayerStatSystemInterface.h>

#define DECLARE_PERFORMANCE_STAT_GROUP(GROUPID, NAME)                                                                                                  \
    {                                                                                                                                      \
        if (auto* statSystem = AZ::Interface<IMultiplayerStatSystem>::Get())                                                               \
        {                                                                                                                                  \
            statSystem->DeclareStatGroup(GROUPID, NAME);                                                                                   \
        }                                                                                                                                  \
        else                                                                                                                               \
        {                                                                                                                                  \
            AZLOG_WARN("DECLARE_PERFORMANCE_STAT_GROUP was called too early. IMultiplayerStatSystem isn't ready yet.");                                \
        }                                                                                                                                  \
    }

#define DECLARE_PERFORMANCE_STAT(GROUPID, STATID, NAME)                                                                                                \
    {                                                                                                                                      \
        if (auto* statSystem = AZ::Interface<IMultiplayerStatSystem>::Get())                                                               \
        {                                                                                                                                  \
            statSystem->DeclareStat(GROUPID, STATID, NAME);                                                                                \
        }                                                                                                                                  \
        else                                                                                                                               \
        {                                                                                                                                  \
            AZLOG_WARN("DECLARE_PERFORMANCE_STAT was called too early. IMultiplayerStatSystem isn't ready yet.");                               \
        }                                                                                                                                  \
    }

#define SET_PERFORMANCE_STAT(STATID, VALUE)                                                                                                \
    {                                                                                                                                      \
        if (auto* statSystem = AZ::Interface<IMultiplayerStatSystem>::Get())                                                               \
        {                                                                                                                                  \
            statSystem->SetStat(STATID, aznumeric_cast<double>(VALUE));                                                                    \
        }                                                                                                                                  \
        else                                                                                                                               \
        {                                                                                                                                  \
            AZLOG_WARN("SET_PERFORMANCE_STAT was called too early. IMultiplayerStatSystem isn't ready yet.");                                   \
        }                                                                                                                                  \
    }

#define INCREMENT_PERFORMANCE_STAT(STATID)                                                                                                 \
    {                                                                                                                                      \
        if (auto* statSystem = AZ::Interface<IMultiplayerStatSystem>::Get())                                                               \
        {                                                                                                                                  \
            statSystem->IncrementStat(STATID);                                                                                             \
        }                                                                                                                                  \
        else                                                                                                                               \
        {                                                                                                                                  \
            AZLOG_WARN("INCREMENT_PERFORMANCE_STAT was called too early. IMultiplayerStatSystem isn't ready yet.");                        \
        }                                                                                                                                  \
    }

#else

#define DECLARE_PERFORMANCE_STAT_GROUP()

#define DECLARE_PERFORMANCE_STAT()

#define SET_PERFORMANCE_STAT()

#define INCREMENT_PERFORMANCE_STAT()

#endif
