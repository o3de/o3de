/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_FRAME_PROFILER_H
#define AZCORE_FRAME_PROFILER_H

#include <AzCore/Driller/DrillerBus.h>
#include <AzCore/std/containers/ring_buffer.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/parallel/config.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    namespace Debug
    {
        namespace FrameProfiler
        {
            /**
             * This structure is used for frame data history, make sure it's memory efficient.
             */
            struct FrameData
            {
                unsigned int    m_frameId;          ///< Id of the frame this data belongs to.
                union
                {
                    ProfilerRegister::TimeData      m_timeData;
                    ProfilerRegister::ValuesData    m_userValues;
                };
            };

            struct RegisterData
            {
                //////////////////////////////////////////////////////////////////////////
                // Profile register snapshot
                /// data that doesn't change
                const char*                     m_name;             ///< Name of the profiler register.
                const char*                     m_function;         ///< Function name in the code.
                int                             m_line;             ///< Line number if the code.
                AZ::u32                         m_systemId;         ///< Register system id.
                ProfilerRegister::Type          m_type;
                RegisterData*                   m_lastParent;       ///< Pointer to the last parent register data.
                AZStd::ring_buffer<FrameData>   m_frames;           ///< History of all frame deltas (basically the data you want to display)
            };

            struct ThreadData
            {
                typedef AZStd::unordered_map<const ProfilerRegister*, RegisterData> RegistersMap;
                AZStd::thread_id    m_id;           ///< Thread id (same as AZStd::thread::id)
                RegistersMap        m_registers;    ///< Map with all the registers (with history)
            };

            typedef AZStd::fixed_vector<ThreadData, Profiler::m_maxNumberOfThreads>  ThreadDataArray;        ///< Array with samplers for all threads
        } // namespace FrameProfiler
    } // namespace Debug
} // namespace AZ

#endif // AZCORE_FRAME_PROFILER_H
#pragma once
