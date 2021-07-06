/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/ProfilerDriller.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/Crc.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/bind/bind.h>

namespace AZ
{
    namespace Debug
    {
        //=========================================================================
        // ProfilerDriller
        // [7/9/2013]
        //=========================================================================
        ProfilerDriller::ProfilerDriller()
        {
            AZStd::ThreadDrillerEventBus::Handler::BusConnect();
        }

        //=========================================================================
        // ~ProfilerDriller
        // [7/9/2013]
        //=========================================================================
        ProfilerDriller::~ProfilerDriller()
        {
            AZStd::ThreadDrillerEventBus::Handler::BusDisconnect();
        }

        //=========================================================================
        // Start
        // [5/24/2013]
        //=========================================================================
        void ProfilerDriller::Start(const Param* params, int numParams)
        {
            for (int i = 0; i < m_numberOfSystemFilters; ++i)
            {
                m_systemFilters[i].desc = "SystemID of the system which counters we are interested in";
                m_systemFilters[i].type = Param::PT_INT;
                m_systemFilters[i].value = 0;
            }

            // Copy valid filters.
            m_numberOfValidFilters = 0;
            if (params)
            {
                for (int i = 0; i < numParams; ++i)
                {
                    if (params[i].type == Param::PT_INT && params[i].value != 0)
                    {
                        m_systemFilters[m_numberOfValidFilters++].value =  params[i].value;
                    }
                }
            }

            // output current threads
            for (ThreadArrayType::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
            {
                OutputThreadEnter(*it);
            }

            ProfilerDrillerBus::Handler::BusConnect();

            if (!Profiler::IsReady())
            {
                Profiler::Create();
            }

            Profiler::AddReference();
        }

        //=========================================================================
        // Stop
        // [5/24/2013]
        //=========================================================================
        void ProfilerDriller::Stop()
        {
            Profiler::ReleaseReference();
            ProfilerDrillerBus::Handler::BusDisconnect();
        }

        //=========================================================================
        // OnError
        // [2/8/2013]
        //=========================================================================
        void ProfilerDriller::Update()
        {
            // \note We could add thread_id in addition to the System ID, but I can't foresee many cases where we would like to profile only a specific thread.
            if (m_numberOfValidFilters)
            {
                for (int iFilter = 0; iFilter < m_numberOfValidFilters; ++iFilter)
                {
                    AZ::u32 systemFilter = *reinterpret_cast<AZ::u32*>(&m_systemFilters[iFilter].value);
                    Profiler::Instance().ReadRegisterValues(AZStd::bind(&ProfilerDriller::ReadProfilerRegisters, this, AZStd::placeholders::_1, AZStd::placeholders::_2), systemFilter);
                }
            }
            else
            {
                Profiler::Instance().ReadRegisterValues(AZStd::bind(&ProfilerDriller::ReadProfilerRegisters, this, AZStd::placeholders::_1, AZStd::placeholders::_2), 0);
            }
        }

        //=========================================================================
        // ReadProfilerRegisters
        // [2/11/2013]
        //=========================================================================
        bool ProfilerDriller::ReadProfilerRegisters(const ProfilerRegister& reg, const AZStd::thread_id& id)
        {
            (void)id;
            m_output->BeginTag(AZ_CRC("ProfilerDriller", 0x172c5268));
            m_output->BeginTag(AZ_CRC("UpdateRegister", 0x6c00b890));
            m_output->Write(AZ_CRC("Id", 0xbf396750), &reg);
            // Send only the data which is changing
            switch (reg.m_type)
            {
            case ProfilerRegister::PRT_TIME:
            {
                m_output->Write(AZ_CRC("Time", 0x6f949845), reg.m_timeData.m_time);
                m_output->Write(AZ_CRC("ChildrenTime", 0x46162d3f), reg.m_timeData.m_childrenTime);
                m_output->Write(AZ_CRC("Calls", 0xdaa35c8f), reg.m_timeData.m_calls);
                m_output->Write(AZ_CRC("ChildrenCalls", 0x6a5a4618), reg.m_timeData.m_childrenCalls);
                m_output->Write(AZ_CRC("ParentId", 0x856a684c), reg.m_timeData.m_lastParent);
            } break;
            case ProfilerRegister::PRT_VALUE:
            {
                m_output->Write(AZ_CRC("Value1", 0xa2756c5a), reg.m_userValues.m_value1);
                m_output->Write(AZ_CRC("Value2", 0x3b7c3de0), reg.m_userValues.m_value2);
                m_output->Write(AZ_CRC("Value3", 0x4c7b0d76), reg.m_userValues.m_value3);
                m_output->Write(AZ_CRC("Value4", 0xd21f98d5), reg.m_userValues.m_value4);
                m_output->Write(AZ_CRC("Value5", 0xa518a843), reg.m_userValues.m_value5);
            } break;
            }
            m_output->EndTag(AZ_CRC("UpdateRegister", 0x6c00b890));
            m_output->EndTag(AZ_CRC("ProfilerDriller", 0x172c5268));

            return true;
        }

        //=========================================================================
        // OnThreadEnter
        // [5/31/2013]
        //=========================================================================
        void ProfilerDriller::OnThreadEnter(const AZStd::thread_id& id, const AZStd::thread_desc* desc)
        {
            m_threads.push_back();
            ThreadInfo& info = m_threads.back();
            info.m_id = (size_t)id.m_id;
            if (desc)
            {
                info.m_name = desc->m_name;
                info.m_cpuId = desc->m_cpuId;
                info.m_priority = desc->m_priority;
                info.m_stackSize = desc->m_stackSize;
            }
            else
            {
                info.m_name = nullptr;
                info.m_cpuId = -1;
                info.m_priority = -100000;
                info.m_stackSize = 0;
            }

            if (m_output)
            {
                OutputThreadEnter(info);
            }
        }

        //=========================================================================
        // OnThreadExit
        // [5/31/2013]
        //=========================================================================
        void ProfilerDriller::OnThreadExit(const AZStd::thread_id& id)
        {
            ThreadArrayType::iterator it = m_threads.begin();
            while (it != m_threads.end())
            {
                if (it->m_id == (size_t)id.m_id)
                {
                    break;
                }
                ++it;
            }

            if (it != m_threads.end())
            {
                if (m_output)
                {
                    OutputThreadExit(*it);
                }

                m_threads.erase(it);
            }
        }

        //=========================================================================
        // OutputThreadEnter
        // [7/9/2013]
        //=========================================================================
        void ProfilerDriller::OutputThreadEnter(const ThreadInfo& threadInfo)
        {
            m_output->BeginTag(AZ_CRC("ProfilerDriller", 0x172c5268));
            m_output->BeginTag(AZ_CRC("ThreadEnter", 0x60e4acfb));
            m_output->Write(AZ_CRC("Id", 0xbf396750), threadInfo.m_id);
            if (threadInfo.m_name)
            {
                m_output->Write(AZ_CRC("Name", 0x5e237e06), threadInfo.m_name);
            }
            m_output->Write(AZ_CRC("CpuId", 0xdf558508), threadInfo.m_cpuId);
            m_output->Write(AZ_CRC("Priority", 0x62a6dc27), threadInfo.m_priority);
            m_output->Write(AZ_CRC("StackSize", 0x9cfaf35b), threadInfo.m_stackSize);
            m_output->EndTag(AZ_CRC("ThreadEnter", 0x60e4acfb));
            m_output->EndTag(AZ_CRC("ProfilerDriller", 0x172c5268));
        }

        //=========================================================================
        // OutputThreadExit
        // [7/9/2013]
        //=========================================================================
        void ProfilerDriller::OutputThreadExit(const ThreadInfo& threadInfo)
        {
            m_output->BeginTag(AZ_CRC("ProfilerDriller", 0x172c5268));
            m_output->BeginTag(AZ_CRC("OnThreadExit", 0x16042db9));
            m_output->Write(AZ_CRC("Id", 0xbf396750), threadInfo.m_id);
            m_output->EndTag(AZ_CRC("OnThreadExit", 0x16042db9));
            m_output->EndTag(AZ_CRC("ProfilerDriller", 0x172c5268));
        }

        //=========================================================================
        // OnRegisterSystem
        // [5/31/2013]
        //=========================================================================
        void ProfilerDriller::OnRegisterSystem(AZ::u32 id, const char* name)
        {
            m_output->BeginTag(AZ_CRC("ProfilerDriller", 0x172c5268));
            m_output->BeginTag(AZ_CRC("RegisterSystem", 0x957739ef));
            m_output->Write(AZ_CRC("Id", 0xbf396750), id);
            m_output->Write(AZ_CRC("Name", 0x5e237e06), name);
            m_output->EndTag(AZ_CRC("RegisterSystem", 0x957739ef));
            m_output->EndTag(AZ_CRC("ProfilerDriller", 0x172c5268));
        }

        //=========================================================================
        // OnUnregisterSystem
        // [5/31/2013]
        //=========================================================================
        void ProfilerDriller::OnUnregisterSystem(AZ::u32 id)
        {
            m_output->BeginTag(AZ_CRC("ProfilerDriller", 0x172c5268));
            m_output->BeginTag(AZ_CRC("UnregisterSystem", 0xa20538e4));
            m_output->Write(AZ_CRC("Id", 0xbf396750), id);
            m_output->EndTag(AZ_CRC("UnregisterSystem", 0xa20538e4));
            m_output->EndTag(AZ_CRC("ProfilerDriller", 0x172c5268));
        }

        //=========================================================================
        // OnNewRegister
        // [5/31/2013]
        //=========================================================================
        void ProfilerDriller::OnNewRegister(const ProfilerRegister& reg, const AZStd::thread_id& threadId)
        {
            m_output->BeginTag(AZ_CRC("ProfilerDriller", 0x172c5268));
            m_output->BeginTag(AZ_CRC("NewRegister", 0xf0f2f287));
            m_output->Write(AZ_CRC("Id", 0xbf396750), &reg);
            m_output->Write(AZ_CRC("ThreadId", 0xd0fd9043), threadId.m_id);
            if (reg.m_name)
            {
                m_output->Write(AZ_CRC("Name", 0x5e237e06), reg.m_name);
            }
            if (reg.m_function)
            {
                m_output->Write(AZ_CRC("Function", 0xcaae163d), reg.m_function);
            }
            m_output->Write(AZ_CRC("Line", 0xd114b4f6), reg.m_line);
            m_output->Write(AZ_CRC("SystemId", 0x0dfecf6f), reg.m_systemId);
            m_output->Write(AZ_CRC("Type", 0x8cde5729), reg.m_type);

            switch (reg.m_type)
            {
            case ProfilerRegister::PRT_TIME:
            {
                m_output->Write(AZ_CRC("Time", 0x6f949845), reg.m_timeData.m_time);
                m_output->Write(AZ_CRC("ChildrenTime", 0x46162d3f), reg.m_timeData.m_childrenTime);
                m_output->Write(AZ_CRC("Calls", 0xdaa35c8f), reg.m_timeData.m_calls);
                m_output->Write(AZ_CRC("ChildrenCalls", 0x6a5a4618), reg.m_timeData.m_childrenCalls);
                m_output->Write(AZ_CRC("ParentId", 0x856a684c), reg.m_timeData.m_lastParent);
            } break;
            case ProfilerRegister::PRT_VALUE:
            {
                m_output->Write(AZ_CRC("Value1", 0xa2756c5a), reg.m_userValues.m_value1);
                m_output->Write(AZ_CRC("Value2", 0x3b7c3de0), reg.m_userValues.m_value2);
                m_output->Write(AZ_CRC("Value3", 0x4c7b0d76), reg.m_userValues.m_value3);
                m_output->Write(AZ_CRC("Value4", 0xd21f98d5), reg.m_userValues.m_value4);
                m_output->Write(AZ_CRC("Value5", 0xa518a843), reg.m_userValues.m_value5);
            } break;
            }
            m_output->EndTag(AZ_CRC("NewRegister", 0xf0f2f287));
            m_output->EndTag(AZ_CRC("ProfilerDriller", 0x172c5268));
        }
    } // namespace Debug
} // namespace AZ
