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
#ifndef INCLUDE_NETWORKGRIDMATEDEBUG_HEADER
#define INCLUDE_NETWORKGRIDMATEDEBUG_HEADER

#pragma once

#ifndef _RELEASE
#    define GRIDMATE_DEBUG_ENABLED 1
#else
#    define GRIDMATE_DEBUG_ENABLED 0
#endif // !_RELEASE

//-----------------------------------------------------------------------------
namespace GridMate
{
    namespace Debug
    {
        const char* const GetAspectNameByBitIndex(size_t aspectBit);

        #if GRIDMATE_DEBUG_ENABLED

        extern int s_DebugDraw;                     // Bound to gm_debugdraw cvar
        extern int s_TraceLevel;                    // Bound to gm_tracelevel cvar
        extern int s_EnableAsserts;                 // Bound to gm_asserts cvar

        enum DebugDrawBits
        {
            Basic           = BIT(0),
            Trace           = BIT(1),
            Stats           = BIT(2),
            Replicas        = BIT(3),
            Actors          = BIT(4),
            EntityDetail    = BIT(5),

            Full            = Basic | Trace | Stats | Replicas | Actors,
            All             = 0xffffffff,
        };

        enum class DebugMessageType
        {
            kTrace,
            kAssert,
        };

        void RegisterCVars();
        void UnregisterCVars();

        void TrackMessage(DebugMessageType type, const char* msg);
        void DebugTrace(bool isAssertFailure, const char* format, ...);

        #endif // GRIDMATE_DEBUG_ENABLED
    } // namespace Debug
} // namespace GridMate

#if GRIDMATE_DEBUG_ENABLED

#define GM_DEBUG_TRACE_LEVEL(level, ...)                                                                 \
    do { if (GridMate::Debug::s_TraceLevel >= level) {GridMate::Debug::DebugTrace(false, __VA_ARGS__); } \
    } while (0);
#define GM_ASSERT_TRACE(c, ...)                                       \
    do { if (!(c)) {GridMate::Debug::DebugTrace(true, __VA_ARGS__); } \
    } while (0);
#define GM_DEBUG_TRACE(...)                 GM_DEBUG_TRACE_LEVEL(1, __VA_ARGS__)

#else // !GRIDMATE_DEBUG_ENABLED

#define GM_DEBUG_TRACE_LEVEL(level, ...)  {; }
#define GM_DEBUG_TRACE(...)               {; }
#define GM_ASSERT_TRACE(c, ...)           {; }

#endif // GRIDMATE_DEBUG_ENABLED

#endif // INCLUDE_NETWORKGRIDMATEDEBUG_HEADER
