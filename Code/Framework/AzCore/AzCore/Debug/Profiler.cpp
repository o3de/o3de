/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Debug/ProfilerDrillerBus.h>

#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/shared_spin_mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/functional.h>
#include <AzCore/Math/Crc.h>

namespace AZ
{
    namespace Debug
    {
        //////////////////////////////////////////////////////////////////////////
        // Globals
        AZStd::chrono::microseconds ProfilerRegister::TimeData::s_startStopOverheadPer1000Calls(0);
        Profiler* Profiler::s_instance = nullptr;
        u64 Profiler::s_id = 0;
        int Profiler::s_useCount = 0;
        //////////////////////////////////////////////////////////////////////////

        /**
         * Profile data stored per thread.
         */
        struct ProfilerThreadData
        {
            static const int m_maxStackSize = 32;
            typedef AZStd::list<ProfilerRegister, OSStdAllocator>   ProfilerRegisterList;
            typedef AZStd::fixed_vector<ProfilerSection*, m_maxStackSize> ProfilerSectionStack;

            AZStd::thread::id                   m_id;               ///< Thread id.
            ProfilerRegisterList                m_registers;        ///< Thread profiler registers (for this thread).
            mutable AZStd::shared_spin_mutex    m_registersLock;    ///< Lock for accessing thread profiler entries. Sadly the only reason for this to exists is so we can read safe the register counters.
            ProfilerSectionStack                m_stack;            ///< Current active sections stack.
        };

        struct ProfilerSystemData
        {
            AZ::u32     m_id;
            const char* m_name;
            bool        m_isActive;
        };

        /**
         * Profiler class data (hidden in from the header file)
         */
        struct ProfilerData
        {
            AZ_CLASS_ALLOCATOR(ProfilerData, OSAllocator, 0);

            AZStd::fixed_vector<ProfilerThreadData, Profiler::m_maxNumberOfThreads>  m_threads;  ///< Array with thread with all belonging information.
            AZStd::shared_spin_mutex                                                m_threadDataMutex;  ///< Spin read/write lock (shared_mutex) for access to the m_threads.
            AZStd::fixed_vector<ProfilerSystemData, Profiler::m_maxNumberOfSystems>  m_systems; ///< Array with systems (profiler/timer groups) that you can enable/disable.
        };

        //=========================================================================
        // Profiler
        // [12/3/2012]
        //=========================================================================
        Profiler::Profiler(const Descriptor& desc)
        {
            (void)desc;
            m_data = aznew ProfilerData;

            // we can periodically call this function (like the end of every frame to refresh the current estimation).
            ProfilerRegister::TimerComputeStartStopOverhead();

            // use a timestamp as and id.
            s_id = AZStd::GetTimeUTCMilliSecond();
        }

        //=========================================================================
        // ~Profiler
        // [12/3/2012]
        //=========================================================================
        Profiler::~Profiler()
        {
            AZ_Assert(s_useCount == 0, "You deleted the profiler while it's still in use.");
            s_id = 0;
            delete m_data;
        }

        //=========================================================================
        // Create
        // [12/3/2012]
        //=========================================================================
        bool Profiler::Create(const Descriptor& desc)
        {
            AZ_Assert(s_instance == nullptr, "Profiler is already created!");
            if (s_instance != nullptr)
            {
                return false;
            }
            s_instance = azcreate(Profiler, (desc), AZ::OSAllocator, "Profiler", 0);
            return true;
        }

        //=========================================================================
        // Destroy
        // [12/3/2012]
        //=========================================================================
        void Profiler::Destroy()
        {
            AZ_Assert(s_instance != nullptr, "Profiler not created");
            if (s_instance)
            {
                azdestroy(s_instance, AZ::OSAllocator);
                s_instance = nullptr;
            }
        }

        //=========================================================================
        // AddReference
        // [5/24/2013]
        //=========================================================================
        void Profiler::AddReference()
        {
            ++s_useCount;
        }

        //=========================================================================
        //
        // [5/24/2013]
        //=========================================================================
        void Profiler::ReleaseReference()
        {
            AZ_Assert(s_useCount > 0, "Use count is already 0, you can't release it!");
            --s_useCount;
            if (s_useCount == 0)
            {
                Destroy();
            }
        }

        //=========================================================================
        // RegisterSystem
        // [12/3/2012]
        //=========================================================================
        bool Profiler::RegisterSystem(AZ::u32 systemId, const char* name, bool isActive)
        {
            for (size_t i = 0; i < m_data->m_systems.size(); ++i)
            {
                if (m_data->m_systems[i].m_id == systemId)
                {
                    return false;
                }
            }
            ProfilerSystemData sd;
            sd.m_id = systemId;
            sd.m_isActive = isActive;
            sd.m_name = name;
            m_data->m_systems.push_back(sd);
            return true;
        }

        //=========================================================================
        // UnregisterSystem
        // [12/3/2012]
        //=========================================================================
        bool Profiler::UnregisterSystem(AZ::u32 systemId)
        {
            size_t i = 0;
            for (; i < m_data->m_systems.size(); ++i)
            {
                ProfilerSystemData& sd = m_data->m_systems[i];
                if (sd.m_id == systemId)
                {
                    if (sd.m_isActive)
                    {
                        DeactivateSystem(sd.m_name);
                    }
                    // Make sure we triggest driller message when the m_threadDataMutex is NOT locked
                    // as we lock them in reverse order when we update the profile driller.
                    AZ_Assert(false, "Currently this code is unused. If we do use it, we should make call EBUS even outside of this function. Where m_threadDataMutex is NOT locked!");
                    //EBUS_DBG_EVENT(ProfilerDrillerBus,OnUnregisterSystem,systemId);
                    break;
                }
            }
            if (i < m_data->m_systems.size())
            {
                m_data->m_systems.erase(m_data->m_systems.begin() + i);
                return true;
            }
            return false;
        }

        //=========================================================================
        // ActivateSystem
        // [12/3/2012]
        //=========================================================================
        bool Profiler::SetSystemState(AZ::u32 systemId, bool isActive)
        {
            for (size_t i = 0; i < m_data->m_systems.size(); ++i)
            {
                ProfilerSystemData& sd = m_data->m_systems[i];
                if (sd.m_id == systemId)
                {
                    if (sd.m_isActive != isActive)
                    {
                        sd.m_isActive = isActive;
                        size_t numThreads = m_data->m_threads.size();
                        for (size_t j = 0; j < numThreads; ++j)
                        {
                            ProfilerThreadData& data = m_data->m_threads[j];
                            ProfilerThreadData::ProfilerRegisterList::iterator it = data.m_registers.begin();
                            ProfilerThreadData::ProfilerRegisterList::iterator end = data.m_registers.end();
                            for (; it != end; ++it)
                            {
                                ProfilerRegister& reg = *it;
                                // This is a big question since this is the only writer
                                // and the timers are readers and value should be read atomically (1 byte)
                                // we should be safe without a synchronization here (as data should be written as we exit)
                                // at worst we can put a volatile in front of the bool. Either way this should not cause
                                // crashes or anything
                                if (reg.m_systemId == systemId)
                                {
                                    reg.m_isActive = isActive ? 1 : 0;
                                }
                            }
                        }
                    }
                    return true;
                }
            }
            return false;
        }

        //=========================================================================
        // ActivateSystem
        // [5/24/2013]
        //=========================================================================
        void Profiler::ActivateSystem(const char* systemName)
        {
            AZ::u32 systemId = AZ::Crc32(systemName);
            bool isNewSystem = false;
            {
                AZStd::unique_lock<AZStd::shared_spin_mutex> writeLock(m_data->m_threadDataMutex);
                if (!SetSystemState(systemId, true))
                {
                    // if the system is new add it
                    RegisterSystem(systemId, systemName, true);
                    isNewSystem = true;
                }
            }
            if (isNewSystem)
            {
                // Make sure we triggest driller message when the m_threadDataMutex is NOT locked
                // as we lock them in reverse order when we update the profile driller.
                EBUS_DBG_EVENT(ProfilerDrillerBus, OnRegisterSystem, systemId, systemName);
            }
        }

        //=========================================================================
        // DeactivateSystem
        // [5/24/2013]
        //=========================================================================
        void Profiler::DeactivateSystem(const char* systemName)
        {
            AZ::u32 systemId = AZ::Crc32(systemName);
            bool isNewSystem = false;
            {
                AZStd::unique_lock<AZStd::shared_spin_mutex> writeLock(m_data->m_threadDataMutex);
                if (!SetSystemState(systemId, false))
                {
                    // if the system is new add it
                    RegisterSystem(systemId, systemName, false);
                    isNewSystem = true;
                }
            }
            if (isNewSystem)
            {
                // Make sure we triggest driller message when the m_threadDataMutex is NOT locked
                // as we lock them in reverse order when we update the profile driller.
                EBUS_DBG_EVENT(ProfilerDrillerBus, OnRegisterSystem, systemId, systemName);
            }
        }


        //=========================================================================
        // IsSystemActive
        // [5/24/2013]
        //=========================================================================
        bool Profiler::IsSystemActive(const char* systemName) const
        {
            return IsSystemActive(AZ::Crc32(systemName));
        }

        //=========================================================================
        // IsSystemActive
        // [12/3/2012]
        //=========================================================================
        bool Profiler::IsSystemActive(AZ::u32 systemId) const
        {
            for (size_t i = 0; i < m_data->m_systems.size(); ++i)
            {
                if (m_data->m_systems[i].m_id == systemId)
                {
                    return m_data->m_systems[i].m_isActive != 0;
                }
            }
            return false;
        }

        //=========================================================================
        // GetNumberOfSystems
        // [12/3/2012]
        //=========================================================================
        int Profiler::GetNumberOfSystems() const
        {
            return static_cast<int>(m_data->m_systems.size());
        }

        //=========================================================================
        // GetSystemName
        // [12/3/2012]
        //=========================================================================
        const char* Profiler::GetSystemName(int index) const
        {
            return m_data->m_systems[index].m_name;
        }

        //=========================================================================
        // GetSystemName
        // [12/3/2012]
        //=========================================================================
        const char* Profiler::GetSystemName(AZ::u32 systemId) const
        {
            for (size_t i = 0; i < m_data->m_systems.size(); ++i)
            {
                if (m_data->m_systems[i].m_id == systemId)
                {
                    return m_data->m_systems[i].m_name;
                }
            }
            return NULL;
        }

        //=========================================================================
        // RemoveThreadData
        // [12/3/2012]
        //=========================================================================
        void Profiler::RemoveThreadData(AZStd::thread_id id)
        {
            // this is very tricky we must be sure that thread is no longer operational
            // otherwise we will crash badly. We can do only because nobody should
            // reference this registers but the thread local data.
            AZStd::unique_lock<AZStd::shared_spin_mutex> writeLock(m_data->m_threadDataMutex);
            size_t numThreads = m_data->m_threads.size();
            ProfilerThreadData* threadData = NULL;
            for (size_t i = 0; i < numThreads; ++i)
            {
                ProfilerThreadData& data = m_data->m_threads[i];
                if (data.m_id == id)
                {
                    threadData = &data;
                    break;
                }
            }

            if (threadData)
            {
                // delete all registers, we can remove the thread data too, but we will need to switch the structure to list
                // so far this is super minimal overhead, registers are more.
                threadData->m_registers.clear();
            }
        }

        //=========================================================================
        // ReadRegisterValues
        // [12/3/2012]
        //=========================================================================
        void Profiler::ReadRegisterValues(const ReadProfileRegisterCB& callback, AZ::u32 systemFilter, const AZStd::thread_id* threadFilter) const
        {
            AZStd::shared_lock<AZStd::shared_spin_mutex> readLock(s_instance->m_data->m_threadDataMutex);
            size_t numThreads = Profiler::s_instance->m_data->m_threads.size();
            for (size_t i = 0; i < numThreads; ++i)
            {
                const ProfilerThreadData& data = s_instance->m_data->m_threads[i];
                if (threadFilter && *threadFilter != data.m_id)
                {
                    continue;
                }
                {
                    AZStd::shared_lock<AZStd::shared_spin_mutex> registersLock(data.m_registersLock);
                    ProfilerThreadData::ProfilerRegisterList::const_iterator it = data.m_registers.begin();
                    ProfilerThreadData::ProfilerRegisterList::const_iterator end = data.m_registers.end();
                    for (; it != end; ++it)
                    {
                        const ProfilerRegister& reg = *it;
                        if (!reg.m_isActive || (systemFilter != 0 && systemFilter != reg.m_systemId))
                        {
                            continue;
                        }
                        if (!callback(reg, data.m_id))
                        {
                            return;
                        }
                    }
                }
            }
        }

        //=========================================================================
        // ResetRegisters
        // [12/4/2012]
        //=========================================================================
        void Profiler::ResetRegisters()
        {
            AZStd::unique_lock<AZStd::shared_spin_mutex> writeLock(s_instance->m_data->m_threadDataMutex);
            size_t numThreads = Profiler::s_instance->m_data->m_threads.size();
            for (size_t i = 0; i < numThreads; ++i)
            {
                ProfilerThreadData& data = s_instance->m_data->m_threads[i];
                {
                    AZStd::unique_lock<AZStd::shared_spin_mutex> registersLock(data.m_registersLock);
                    ProfilerThreadData::ProfilerRegisterList::iterator it = data.m_registers.begin();
                    ProfilerThreadData::ProfilerRegisterList::iterator end = data.m_registers.end();
                    for (; it != end; ++it)
                    {
                        ProfilerRegister& reg = *it;
                        reg.Reset();
                    }
                }
            }

            // Reset registers event
        }

        //=========================================================================
        // CreateRegister
        // [6/28/2013]
        //=========================================================================
        ProfilerRegister*
        ProfilerRegister::CreateRegister(const char* systemName, const char* name, const char* function, int line, ProfilerRegister::Type type)
        {
            static AZ_THREAD_LOCAL ProfilerThreadData* threadData = nullptr;
            static AZ_THREAD_LOCAL u64 profilerId = 0;
            if (profilerId != Profiler::s_id)
            {
                threadData = nullptr; // profiler has changed
                profilerId = Profiler::s_id;
            }
            AZ::u32 systemId = AZ::Crc32(systemName);
            ProfilerRegister* reg;
            {
                AZStd::unique_lock<AZStd::shared_spin_mutex> writeLock(Profiler::s_instance->m_data->m_threadDataMutex);

                // make sure we have the system registered. This function will just return false if the system exists.
                if (systemName)
                {
                    Profiler::s_instance->RegisterSystem(systemId, systemName, true);
                }

                if (threadData == nullptr)  // if this is a new thread add the data
                {
                    AZStd::thread::id threadId = AZStd::this_thread::get_id();
                    Profiler::s_instance->m_data->m_threads.push_back();
                    threadData = &Profiler::s_instance->m_data->m_threads.back();
                    threadData->m_id = threadId;
                }
                threadData->m_registers.push_back();
                reg = &threadData->m_registers.back();
                reg->m_name = name;
                reg->m_function = function;
                reg->m_line = line;
                reg->m_systemId = systemId;
                reg->m_isActive = Profiler::s_instance->IsSystemActive(systemId) ? 1 : 0;
                reg->m_type = type;
                reg->m_threadData = threadData;

                reg->Reset();
            }
            // Make sure we triggest driller message when the m_threadDataMutex is NOT locked
            // as we lock them in reverse order when we update the profile driller.
            EBUS_DBG_EVENT(ProfilerDrillerBus, OnNewRegister, *reg, threadData->m_id);

            return reg;
        }

        //=========================================================================
        // TimerCreateAndStart
        // [11/30/2012]
        //=========================================================================
        ProfilerRegister*
        ProfilerRegister::TimerCreateAndStart(const char* systemName, const char* name, ProfilerSection * section, const char* function, int line)
        {
            AZStd::chrono::system_clock::time_point start = AZStd::chrono::system_clock::now();
            ProfilerRegister* reg = CreateRegister(systemName, name, function, line, ProfilerRegister::PRT_TIME);
            AZStd::chrono::system_clock::time_point end = AZStd::chrono::system_clock::now();

            // adjust the parent timer with the overhead we incur during timer operations. (TODO with TLS this is so fast that we might not need to do it)
            if (!reg->m_threadData->m_stack.empty())  // if we are not he last element
            {
                AZStd::chrono::microseconds elapsed = end - start;
                reg->m_threadData->m_stack.back()->m_childTime += elapsed;  // no need to check if we go in the future as this will happen on Stop
            }

            if (reg->m_isActive)
            {
                section->m_register = reg;
                section->m_start = end;
                reg->m_threadData->m_stack.push_back(section);
            }
            else
            {
                section->m_register = nullptr;
            }

            return reg;
        }

        //=========================================================================
        // ValueCreate
        // [6/28/2013]
        //=========================================================================
        ProfilerRegister*
        ProfilerRegister::ValueCreate(const char* systemName, const char* name, const char* function, int line)
        {
            return CreateRegister(systemName, name, function, line, ProfilerRegister::PRT_VALUE);
        }

        //=========================================================================
        // TimerStart
        // [11/29/2012]
        //=========================================================================
        void ProfilerRegister::TimerStart(ProfilerSection* section)
        {
            ProfilerRegister* reg = this;
            if (reg->m_isActive)
            {
                section->m_register = reg;
                reg->m_threadData->m_stack.push_back(section);
                section->m_start =  AZStd::chrono::system_clock::now();
            }
            else
            {
                section->m_register = nullptr;
            }
        }

        //=========================================================================
        // TimerStop
        // [11/29/2012]
        //=========================================================================
        void ProfilerRegister::TimerStop()
        {
            AZStd::chrono::system_clock::time_point end = AZStd::chrono::system_clock::now();
            ProfilerSection* section = m_threadData->m_stack.back();
            AZStd::chrono::microseconds elapsedTime = end - section->m_start;
            {
                m_threadData->m_registersLock.lock(); // lock for write
                ++m_timeData.m_calls;
                m_timeData.m_time += elapsedTime.count();
                m_timeData.m_childrenTime += section->m_childTime.count();
                m_timeData.m_childrenCalls += section->m_childCalls;
                m_threadData->m_registersLock.unlock(); // unlock
            }
            m_threadData->m_stack.pop_back();

            // adjust the parent timer with the overhead we incur during timer operations.
            if (!m_threadData->m_stack.empty())
            {
                ProfilerSection* parent = m_threadData->m_stack.back();
                m_timeData.m_lastParent = parent->m_register;
                parent->m_childTime += elapsedTime /*+ s_startStopOverhead*/;  // add the overhead since most of it is in Stop()
                ++parent->m_childCalls;
            }
        }

        //=========================================================================
        // Reset
        // [12/4/2012]
        //=========================================================================
        void ProfilerRegister::Reset()
        {
            switch (m_type)
            {
            case PRT_TIME:
            {
                m_timeData.m_time = 0;
                m_timeData.m_childrenTime = 0;
                m_timeData.m_calls = 0;
                m_timeData.m_childrenCalls = 0;
                m_timeData.m_lastParent = nullptr;
            } break;
            case PRT_VALUE:
            {
                m_userValues.m_value1 = 0;
                m_userValues.m_value2 = 0;
                m_userValues.m_value3 = 0;
                m_userValues.m_value4 = 0;
                m_userValues.m_value5 = 0;
            } break;
            }
        }

        //=========================================================================
        // ComputeStartStopOverhead
        // [12/3/2012]
        //=========================================================================
        void ProfilerRegister::TimerComputeStartStopOverhead()
        {
            // compute default thread start stop overhead
            ProfilerThreadData sampleThreadData;
            sampleThreadData.m_id = AZStd::this_thread::get_id();
            sampleThreadData.m_registers.push_back();
            ProfilerRegister& sampleRegister = sampleThreadData.m_registers.back();
            sampleRegister.m_isActive = true;
            sampleRegister.m_name = nullptr;
            sampleRegister.m_systemId = 0;
            sampleRegister.m_threadData = &sampleThreadData;

            const int numSamples = 1000;
            for (int iRepetition = 0; iRepetition < 1000; ++iRepetition) // just for test
            {
                ProfilerSection section;
                sampleRegister.TimerStart(&section);
                AZStd::chrono::system_clock::time_point start = AZStd::chrono::system_clock::now();
                for (int i = 0; i < numSamples; ++i)
                {
                    static AZ::Debug::ProfilerRegister* sampleRegisterPtr = &sampleRegister; // the creation is timed differently
                    ProfilerSection subSection;
                    if (sampleRegisterPtr != NULL)
                    {
                        sampleRegister.TimerStart(&subSection);
                    }
                }
                AZStd::chrono::microseconds elapsed = (AZStd::chrono::system_clock::now() - start);
                if (TimeData::s_startStopOverheadPer1000Calls.count() == 0)  // if first time set otherwise smooth average
                {
                    TimeData::s_startStopOverheadPer1000Calls = elapsed;
                }
                else
                {
                    float fNew = static_cast<float>(elapsed.count());
                    float fCurrent = static_cast<float>(TimeData::s_startStopOverheadPer1000Calls.count());
                    int deltaValue = static_cast<int>((fNew - fCurrent) * 0.1f);
                    if (deltaValue < 0)
                    {
                        TimeData::s_startStopOverheadPer1000Calls -= AZStd::chrono::microseconds(-deltaValue);
                    }
                    else
                    {
                        TimeData::s_startStopOverheadPer1000Calls += AZStd::chrono::microseconds(deltaValue);
                    }
                }
            }
            //AZ_TracePrintf("Profiler","Overhead %d microseconds per 1000 profile calls!\n",TimeData::s_startStopOverheadPer1000Calls.count());
        }

    }
} // namespace AZ
