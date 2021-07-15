/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/Debug/Profiler.h>

#define SUBSYSTEM_DEFINES \
    X(PROFILE_ANY,          "Any")          \
    X(PROFILE_RENDERER,     "Renderer")     \
    X(PROFILE_3DENGINE,     "3DEngine")     \
    X(PROFILE_PARTICLE,     "Particle")     \
    X(PROFILE_AI,           "AI")           \
    X(PROFILE_ANIMATION,    "Animation")    \
    X(PROFILE_MOVIE,        "Movie")        \
    X(PROFILE_ENTITY,       "Entity")       \
    X(PROFILE_UI,           "UI")           \
    X(PROFILE_NETWORK,      "Network")      \
    X(PROFILE_PHYSICS,      "Physics")      \
    X(PROFILE_SCRIPT,       "Script")       \
    X(PROFILE_SCRIPT_CFUNC, "Script C Functions")  \
    X(PROFILE_AUDIO,        "Audio")        \
    X(PROFILE_EDITOR,       "Editor")       \
    X(PROFILE_SYSTEM,       "System")       \
    X(PROFILE_ACTION,       "Action")       \
    X(PROFILE_GAME,         "Game")         \
    X(PROFILE_INPUT,        "Input")        \
    X(PROFILE_SYNC,         "Sync")         \
    X(PROFILE_NETWORK_TRAFFIC, "Network Traffic") \
    X(PROFILE_DEVICE,       "Device")

#define X(Subsystem, SubsystemName) Subsystem,
enum EProfiledSubsystem
{
    SUBSYSTEM_DEFINES
    PROFILE_LAST_SUBSYSTEM
};
#undef X

static_assert(static_cast<AZ::Debug::ProfileCategory>(PROFILE_LAST_SUBSYSTEM) == AZ::Debug::ProfileCategory::LegacyLast, "Mismatched AZ and Legacy profile categories");
#include <AzCore/Debug/EventTrace.h>



#define FUNCTION_PROFILER_LEGACYONLY(pISystem, subsystem)

#define FUNCTION_PROFILER(pISystem, subsystem) \
    AZ_PROFILE_FUNCTION(static_cast<AZ::Debug::ProfileCategory>(subsystem));

#define FUNCTION_PROFILER_FAST(pISystem, subsystem, bProfileEnabled) \
    AZ_PROFILE_FUNCTION(static_cast<AZ::Debug::ProfileCategory>(subsystem));

#define FUNCTION_PROFILER_ALWAYS(pISystem, subsystem) \
    AZ_PROFILE_FUNCTION(static_cast<AZ::Debug::ProfileCategory>(subsystem));

#define FRAME_PROFILER_LEGACYONLY(szProfilerName, pISystem, subsystem)

#define FRAME_PROFILER(szProfilerName, pISystem, subsystem) \
    AZ_PROFILE_SCOPE(static_cast<AZ::Debug::ProfileCategory>(subsystem), szProfilerName);

#define FRAME_PROFILER_FAST(szProfilerName, pISystem, subsystem, bProfileEnabled) \
    AZ_PROFILE_SCOPE(static_cast<AZ::Debug::ProfileCategory>(subsystem), szProfilerName);

#define FUNCTION_PROFILER_SYS(subsystem) \
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_##subsystem)

#define STALL_PROFILER(cause)

