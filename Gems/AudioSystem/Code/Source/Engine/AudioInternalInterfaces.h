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

#include <platform.h>

#if !defined(AUDIO_RELEASE)
    #include <sstream>
#endif // !AUDIO_RELEASE

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EAudioEventResult : TATLEnumFlagsType
    {
        eAER_NONE           = 0,
        eAER_SUCCESS        = 1,
        eAER_FAILED         = 2,
        eAER_STILL_LOADING  = 3,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EAudioRequestInfoFlags : TATLEnumFlagsType
    {
        eARIF_NONE                  = 0,
        eARIF_WAITING_FOR_REMOVAL   = AUDIO_BIT(0),
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioEventListener
    {
        SAudioEventListener()
            : m_callbackOwner(nullptr)
            , m_fnOnEvent(nullptr)
            , m_requestType(eART_AUDIO_ALL_REQUESTS)
            , m_specificRequestMask(ALL_AUDIO_REQUEST_SPECIFIC_TYPE_FLAGS)
        {}

        SAudioEventListener(const SAudioEventListener& other)
            : m_callbackOwner(other.m_callbackOwner)
            , m_fnOnEvent(other.m_fnOnEvent)
            , m_requestType(other.m_requestType)
            , m_specificRequestMask(other.m_specificRequestMask)
        {}

        SAudioEventListener& operator=(const SAudioEventListener& other)
        {
            m_callbackOwner = other.m_callbackOwner;
            m_fnOnEvent = other.m_fnOnEvent;
            m_requestType = other.m_requestType;
            m_specificRequestMask = other.m_specificRequestMask;
            return *this;
        }

        bool operator==(const SAudioEventListener& rhs) const
        {
            return (m_callbackOwner == rhs.m_callbackOwner
                && m_fnOnEvent == rhs.m_fnOnEvent
                && m_requestType == rhs.m_requestType
                && m_specificRequestMask == rhs.m_specificRequestMask);
        }

        const void* m_callbackOwner;
        AudioRequestCallbackType m_fnOnEvent;
        EAudioRequestType m_requestType;
        TATLEnumFlagsType m_specificRequestMask;
    };

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
