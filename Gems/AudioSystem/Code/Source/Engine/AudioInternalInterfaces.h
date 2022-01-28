/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <IAudioSystem.h>
#include <IAudioSystemImplementation.h>


namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(AUDIO_RELEASE)
    // Filter for drawing debug info to the screen
    namespace DebugDraw
    {
        enum Options : AZ::u32
        {
            None = 0,
            DrawObjects = (1 << 0),
            ObjectLabels = (1 << 1),
            ObjectTriggers = (1 << 2),
            ObjectStates = (1 << 3),
            ObjectRtpcs = (1 << 4),
            ObjectEnvironments = (1 << 5),
            DrawRays = (1 << 6),
            RayLabels = (1 << 7),
            DrawListener = (1 << 8),
            ActiveEvents = (1 << 9),
            ActiveObjects = (1 << 10),
            FileCacheInfo = (1 << 11),
            MemoryInfo = (1 << 12),
        };
    }

    namespace FileCacheManagerDebugDraw
    {
        enum Options : AZ::u8
        {
            All = 0,
            Global = (1 << 0),
            LevelSpecific = (1 << 1),
            UseCounted = (1 << 2),
            Loaded = (1 << 3),
        };
    }
#endif // !AUDIO_RELEASE

} // namespace Audio
