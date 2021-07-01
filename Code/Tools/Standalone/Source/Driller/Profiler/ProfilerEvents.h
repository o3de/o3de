/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_PROFILER_EVENTS_H
#define DRILLER_PROFILER_EVENTS_H

#include "Source/Driller/DrillerEvent.h"

namespace Driller
{
    namespace Profiler
    {
        /// Time register data.
        struct TimeData
        {
            AZ::u64 m_time;         ///< Total inclusive time current and children in microseconds.
            AZ::u64 m_childrenTime; ///< Time taken by child profilers in microseconds.
            AZ::s64 m_calls;        ///< Number of calls for this register.
            AZ::s64 m_childrenCalls;///< Number of children calls.
            AZ::u64 m_lastParentRegisterId; ///< Id of the last parent register.
        };

        /// Value register data.
        struct ValuesData
        {
            AZ::s64     m_value1;
            AZ::s64     m_value2;
            AZ::s64     m_value3;
            AZ::s64     m_value4;
            AZ::s64     m_value5;
        };

        /// Data that will change every frame (technically only when registers are called)
        struct RegisterData
        {
            RegisterData()
            {
                m_valueData.m_value1 = 0;
                m_valueData.m_value2 = 0;
                m_valueData.m_value3 = 0;
                m_valueData.m_value4 = 0;
                m_valueData.m_value5 = 0;
            }

            union
            {
                TimeData    m_timeData;
                ValuesData  m_valueData;
            };
        };

        /// Data that never changes
        struct RegisterInfo
        {
            RegisterInfo()
                : m_id(0)
                , m_threadId(0)
                , m_name(nullptr)
                , m_function(nullptr)
                , m_line(-1)
                , m_systemId(0)
            {}

            enum Type
            {
                PRT_TIME = 0,       ///< Time register - RegisterData::m_time data is used.
                PRT_VALUE,          ///< Value register - RegisterData::m_values data is used.
            };

            unsigned char   m_type;     ///< Register type (time or values)
            AZ::u64         m_id;       ///< Register id (technically the pointer during execution).
            AZ::u64         m_threadId; ///< Native thread handle (AZStd::native_thread_id_type) typically a pointer too.
            const char*     m_name;     ///< Name/description of the register it's optional for time registers.
            const char*     m_function; ///< Name of the function which we are sampling.
            int             m_line;     ///< Line in the code where this register is created (start sampling).
            AZ::u32         m_systemId; ///< Crc32 of the system name provided by the user.
        };

        enum ProfilerEventType
        {
            PET_NEW_REGISTER = 0,
            PET_UPDATE_REGISTER,
            PET_ENTER_THREAD,
            PET_EXIT_THREAD,
            PET_REGISTER_SYSTEM,
            PET_UNREGISTER_SYSTEM,
        };
    }

    class ProfilerDrillerNewRegisterEvent;
    class ProfilerDataAggregator;

    class ProfilerDrillerUpdateRegisterEvent
        : public DrillerEvent
    {
        friend class ProfilerDrillerNewRegisterEvent;
        friend class ProfilerDrillerHandlerParser;
    public:
        AZ_CLASS_ALLOCATOR(ProfilerDrillerUpdateRegisterEvent, AZ::SystemAllocator, 0)

        ProfilerDrillerUpdateRegisterEvent()
            : DrillerEvent(Profiler::PET_UPDATE_REGISTER)
            , m_register(nullptr)
            , m_previousSample(nullptr)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        void            PreComputeForward(ProfilerDrillerNewRegisterEvent* newEvt);

        const Profiler::RegisterData&       GetData() const  { return m_registerData; }
        const ProfilerDrillerNewRegisterEvent*  GetRegister() const  { return m_register; }
        const ProfilerDrillerUpdateRegisterEvent*   GetPreviousSample() const { return m_previousSample; }
        const AZ::u64 GetRegisterId() const { return m_registerId; }

    private:
        AZ::u64                                 m_registerId;       ///< Id if the register.
        Profiler::RegisterData                  m_registerData;     ///< Register sample data.
        ProfilerDrillerNewRegisterEvent*        m_register;         ///< Cached pointer to the register.
        ProfilerDrillerUpdateRegisterEvent*     m_previousSample;   ///< Pointer to the previous register values (null if this is the first sample).
    };

    class ProfilerDrillerNewRegisterEvent
        : public DrillerEvent
    {
        friend class ProfilerDrillerUpdateRegisterEvent;
        friend class ProfilerDrillerHandlerParser;
    public:
        AZ_CLASS_ALLOCATOR(ProfilerDrillerNewRegisterEvent, AZ::SystemAllocator, 0)

        ProfilerDrillerNewRegisterEvent()
            : DrillerEvent(Profiler::PET_NEW_REGISTER)
            , m_lastUpdate(nullptr)
            , m_lastPrecomputed(nullptr)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        const Profiler::RegisterData&       GetData() const { return m_lastUpdate ? m_lastUpdate->m_registerData : m_registerData; }
        const Profiler::RegisterInfo&       GetInfo() const { return m_registerInfo; }
        const ProfilerDrillerUpdateRegisterEvent*   GetLastSample() const { return m_lastUpdate; }

    private:
        Profiler::RegisterInfo      m_registerInfo;     ///< Register information.
        Profiler::RegisterData      m_registerData;     ///< Register sample data.

        // m_lastUpdate is actually also the current scrubber frame for that register.
        ProfilerDrillerUpdateRegisterEvent* m_lastUpdate;   ///< Pointer to the last set of RegisterData (null if there is not last set)

        // because we precompute a small number of registers in order to show the note track in the main view, we need
        // a seperate pointer to the prior precomputed data.
        ProfilerDrillerUpdateRegisterEvent* m_lastPrecomputed;
    };

    class ProfilerDrillerEnterThreadEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(ProfilerDrillerEnterThreadEvent, AZ::SystemAllocator, 0)

        ProfilerDrillerEnterThreadEvent()
            : DrillerEvent(Profiler::PET_ENTER_THREAD)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        AZ::u64                     m_threadId;
        const char*                 m_threadName;   ///< Debug name of thread if one is provided.
        AZ::s32                     m_cpuId;        ///< If of the CPU where this thread should run.
        AZ::s32                     m_priority;
        AZ::u32                     m_stackSize;
    };

    class ProfilerDrillerExitThreadEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(ProfilerDrillerExitThreadEvent, AZ::SystemAllocator, 0)

        ProfilerDrillerExitThreadEvent()
            : DrillerEvent(Profiler::PET_EXIT_THREAD)
            , m_threadData(nullptr)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        AZ::u64                             m_threadId;
        ProfilerDrillerEnterThreadEvent*    m_threadData;
    };

    class ProfilerDrillerRegisterSystemEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(ProfilerDrillerRegisterSystemEvent, AZ::SystemAllocator, 0)

        ProfilerDrillerRegisterSystemEvent()
            : DrillerEvent(Profiler::PET_REGISTER_SYSTEM)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        AZ::u32                     m_systemId;
        const char*                 m_name;         ///< Debug name of thread system.
    };

    class ProfilerDrillerUnregisterSystemEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(ProfilerDrillerUnregisterSystemEvent, AZ::SystemAllocator, 0)

        ProfilerDrillerUnregisterSystemEvent()
            : DrillerEvent(Profiler::PET_UNREGISTER_SYSTEM)
            , m_systemData(nullptr)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        AZ::u32                             m_systemId;
        ProfilerDrillerRegisterSystemEvent* m_systemData;
    };
}

#endif
