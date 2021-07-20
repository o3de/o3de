/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_PROFILER_DRILLER_H
#define AZCORE_PROFILER_DRILLER_H 1

#include <AzCore/Driller/Driller.h>
#include <AzCore/Debug/ProfilerDrillerBus.h>
#include <AzCore/std/parallel/threadbus.h>

namespace AZStd
{
    struct thread_id;
    struct thread_desc;
}

namespace AZ
{
    namespace Debug
    {
        struct ProfilerSystemData;
        class ProfilerRegister;

        /**
         * ProfilerDriller or we can just make a Profiler driller and read the registers ourself.
         */
        class ProfilerDriller
            : public Driller
            , public ProfilerDrillerBus::Handler
            , public AZStd::ThreadDrillerEventBus::Handler
        {
            struct ThreadInfo
            {
                AZ::u64         m_id;
                AZ::u32         m_stackSize;
                AZ::s32         m_priority;
                AZ::s32         m_cpuId;
                const char*    m_name;
            };
            typedef vector<ThreadInfo>::type ThreadArrayType;

        public:

            AZ_CLASS_ALLOCATOR(ProfilerDriller, OSAllocator, 0)

            ProfilerDriller();
            virtual ~ProfilerDriller();

        protected:
            //////////////////////////////////////////////////////////////////////////
            // Driller
            virtual const char*  GroupName() const          { return "SystemDrillers"; }
            virtual const char*  GetName() const            { return "ProfilerDriller"; }
            virtual const char*  GetDescription() const     { return "Collects data from all available profile registers."; }
            virtual int          GetNumParams() const       { return m_numberOfSystemFilters; }
            virtual const Param* GetParam(int index) const  { AZ_Assert(index >= 0 && index < m_numberOfSystemFilters, "Invalid index"); return &m_systemFilters[index]; }
            virtual void         Start(const Param* params = NULL, int numParams = 0);
            virtual void         Stop();
            virtual void         Update();
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Thread driller event bus
            /// Called when we enter a thread, optional thread_desc is provided when the use provides one.
            virtual void OnThreadEnter(const AZStd::thread_id& id, const AZStd::thread_desc* desc);
            /// Called when we exit a thread.
            virtual void OnThreadExit(const AZStd::thread_id& id);

            /// Output thread enter to stream.
            void OutputThreadEnter(const ThreadInfo& threadInfo);
            /// Output thread exit to stream.
            void OutputThreadExit(const ThreadInfo& threadInfo);
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Profiler Driller bus
            virtual void OnRegisterSystem(AZ::u32 id, const char* name);

            virtual void OnUnregisterSystem(AZ::u32 id);

            virtual void OnNewRegister(const ProfilerRegister& reg, const AZStd::thread_id& threadId);
            //////////////////////////////////////////////////////////////////////////

            /// Read profile registers callback.
            bool ReadProfilerRegisters(const ProfilerRegister& reg, const AZStd::thread_id& id);



            static const int m_numberOfSystemFilters = 16;
            int              m_numberOfValidFilters = 0 ;    ///< Number of valid filter set when the driller was created.
            Param            m_systemFilters[m_numberOfSystemFilters]; ///< If != 0, it's a ID of specific System we would like to drill.
            ThreadArrayType  m_threads;
        };
    }
} // namespace AZ

#endif // AZCORE_PROFILER_DRILLER_H
#pragma once
