/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/FrameProfilerComponent.h>
#include <AzCore/Debug/FrameProfilerBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzCore/Debug/Profiler.h>

namespace AZ
{
    namespace Debug
    {
        //=========================================================================
        // FrameProfilerComponent
        // [12/5/2012]
        //=========================================================================
        FrameProfilerComponent::FrameProfilerComponent()
            : m_numFramesStored(2)
            , m_frameId(0)
            , m_pauseOnFrame(0)
            , m_currentThreadData(NULL)
        {
        }

        //=========================================================================
        // ~FrameProfilerComponent
        // [12/5/2012]
        //=========================================================================
        FrameProfilerComponent::~FrameProfilerComponent()
        {
        }

        //=========================================================================
        // Activate
        // [12/5/2012]
        //=========================================================================
        void FrameProfilerComponent::Activate()
        {
            if (!Profiler::IsReady())
            {
                Profiler::Create();
            }

            Profiler::AddReference();

            TickBus::Handler::BusConnect();
            AZ_Assert(m_numFramesStored >= 1, "We must have at least one frame to store, otherwise this component is useless!");
        }

        //=========================================================================
        // Deactivate
        // [12/5/2012]
        //=========================================================================
        void FrameProfilerComponent::Deactivate()
        {
            TickBus::Handler::BusDisconnect();

            Profiler::ReleaseReference();
        }

        //=========================================================================
        // OnTick
        // [12/5/2012]
        //=========================================================================
        void FrameProfilerComponent::OnTick(float deltaTime, ScriptTimePoint time)
        {
            (void)deltaTime;
            (void)time;
            ++m_frameId;
            AZ_Error("Profiler", m_frameId != m_pauseOnFrame, "Triggered user pause/error on this frame! Check FrameProfilerComponent pauseOnFrame value!");

            if (!Profiler::IsReady())
            {
                return;                       // we can't sample registers without profiler
            }
            // collect data from the profiler
            m_currentThreadData = NULL;
            Profiler::Instance().ReadRegisterValues(AZStd::bind(&FrameProfilerComponent::ReadProfilerRegisters, this, AZStd::placeholders::_1, AZStd::placeholders::_2));

            // process all the resulting data here, not while reading the registers
            for (size_t iThread = 0; iThread < m_threads.size(); ++iThread)
            {
                FrameProfiler::ThreadData& td = m_threads[iThread];
                FrameProfiler::ThreadData::RegistersMap::iterator it = td.m_registers.begin();
                FrameProfiler::ThreadData::RegistersMap::iterator last = td.m_registers.end();
                for (; it != last; ++it)
                {
                    // fix up parents
                    FrameProfiler::RegisterData& rd = it->second;
                    if (rd.m_type == ProfilerRegister::PRT_TIME)
                    {
                        const FrameProfiler::FrameData& fd = rd.m_frames.back();
                        if (fd.m_timeData.m_lastParent != nullptr)
                        {
                            FrameProfiler::ThreadData::RegistersMap::iterator parentIt = td.m_registers.find(fd.m_timeData.m_lastParent);
                            AZ_Assert(parentIt != td.m_registers.end(), "We have a parent register that is not in our register map. This should not happen!");
                            rd.m_lastParent = &parentIt->second;
                        }
                        else
                        {
                            rd.m_lastParent = NULL;
                        }
                    }
                }
            }

            // send an even to whomever cares
            EBUS_EVENT(FrameProfilerBus, OnFrameProfilerData, m_threads);
        }

        int FrameProfilerComponent::GetTickOrder()
        {
            // Even it's not critical we should tick last to capture the current frame
            // so TICK_LAST (since it's not the last int +1 is a valid assumption)
            return TICK_LAST + 1;
        }

        //=========================================================================
        // ReadRegisterCallback
        // [12/5/2012]
        //=========================================================================
        bool FrameProfilerComponent::ReadProfilerRegisters(const ProfilerRegister& reg, const AZStd::thread_id& id)
        {
            if (m_currentThreadData == NULL || m_currentThreadData->m_id != id)
            {
                m_currentThreadData = NULL;

                // find the thread and cache it, as we will received registers thread by thread... so we don't search.
                for (size_t i = 0; i < m_threads.size(); ++i)
                {
                    FrameProfiler::ThreadData* td = &m_threads[i];
                    if (td->m_id == id)
                    {
                        m_currentThreadData = td;
                        break;
                    }
                }

                if (m_currentThreadData == NULL)
                {
                    m_threads.push_back();
                    m_currentThreadData = &m_threads.back();
                    m_currentThreadData->m_id = id;
                }
            }

            const ProfilerRegister* profReg = &reg;
            FrameProfiler::ThreadData::RegistersMap::pair_iter_bool pairIterBool = m_currentThreadData->m_registers.insert_key(profReg);
            FrameProfiler::RegisterData& regData = pairIterBool.first->second;

            // now update dynamic data with as little as possible computation (we must be fast)
            FrameProfiler::FrameData fd; // we can actually move this computation (FrameData and push) for later but we will need to use more memory
            fd.m_frameId = m_frameId;

            if (pairIterBool.second)
            {
                // when insert copy the static data only once
                regData.m_name = profReg->m_name;
                regData.m_function = profReg->m_function;
                regData.m_line = profReg->m_line;
                regData.m_systemId = profReg->m_systemId;
                regData.m_frames.set_capacity(m_numFramesStored);
                regData.m_type = static_cast<ProfilerRegister::Type>(profReg->m_type);
            }

            switch (regData.m_type)
            {
            case ProfilerRegister::PRT_TIME:
            {
                fd.m_timeData.m_time = profReg->m_timeData.m_time;
                fd.m_timeData.m_childrenTime = profReg->m_timeData.m_childrenTime;
                fd.m_timeData.m_calls = profReg->m_timeData.m_calls;
                fd.m_timeData.m_childrenCalls = profReg->m_timeData.m_childrenCalls;
                fd.m_timeData.m_lastParent = profReg->m_timeData.m_lastParent;
            } break;
            case ProfilerRegister::PRT_VALUE:
            {
                fd.m_userValues.m_value1 = profReg->m_userValues.m_value1;
                fd.m_userValues.m_value2 = profReg->m_userValues.m_value2;
                fd.m_userValues.m_value3 = profReg->m_userValues.m_value3;
                fd.m_userValues.m_value4 = profReg->m_userValues.m_value4;
                fd.m_userValues.m_value5 = profReg->m_userValues.m_value5;
            } break;
            }

            regData.m_frames.push_back(fd);
            return true;
        }

        //=========================================================================
        // GetProvidedServices
        //=========================================================================
        void FrameProfilerComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("FrameProfilerService", 0x05d1bb90));
        }

        //=========================================================================
        // GetIncompatibleServices
        //=========================================================================
        void FrameProfilerComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("FrameProfilerService", 0x05d1bb90));
        }

        //=========================================================================
        // GetDependentServices
        //=========================================================================
        void FrameProfilerComponent::GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("MemoryService", 0x5c4d473c));
        }

        //=========================================================================
        // Reflect
        //=========================================================================
        void FrameProfilerComponent::Reflect(ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<FrameProfilerComponent, AZ::Component>()
                    ->Version(1)
                    ->Field("numFramesStored", &FrameProfilerComponent::m_numFramesStored)
                    ->Field("pauseOnFrame", &FrameProfilerComponent::m_pauseOnFrame)
                    ;

                if (EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<FrameProfilerComponent>(
                        "Frame Profiler", "Performs per frame profiling (FPS counter, registers, etc.)")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Profiling")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->DataElement(AZ::Edit::UIHandlers::SpinBox, &FrameProfilerComponent::m_numFramesStored, "Number of Frames", "How many frames we will keep with the RUNTIME buffers.")
                            ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->DataElement(AZ::Edit::UIHandlers::SpinBox, &FrameProfilerComponent::m_pauseOnFrame, "Pause on frame", "Paused the engine (debug break) on a specific frame. 0 means no pause!")
                        ;
                }
            }
        }
    } // namespace Debug
} // namespace AZ
