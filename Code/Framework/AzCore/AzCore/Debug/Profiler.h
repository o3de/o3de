/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/function/function_fwd.h>

#ifdef USE_PIX
#include <AzCore/PlatformIncl.h>
#include <WinPixEventRuntime/pix3.h>
#endif

#if defined(AZ_PROFILER_MACRO_DISABLE) // by default we never disable the profiler registers as their overhead should be minimal, you can still do that for your code though.
#   define AZ_PROFILE_SCOPE(...)
#   define AZ_PROFILE_FUNCTION(...)
#   define AZ_PROFILE_BEGIN(...)
#   define AZ_PROFILE_END(...)
#else
/**
 * Macro to declare a profile section for the current scope { }.
 * format is: AZ_PROFILE_SCOPE(categoryName, const char* formatStr, ...)
 */
#   define AZ_PROFILE_SCOPE(category, ...) ::AZ::ProfileScope AZ_JOIN(azProfileScope, __LINE__){ #category, __VA_ARGS__ }
#   define AZ_PROFILE_FUNCTION(category) AZ_PROFILE_SCOPE(category, AZ_FUNCTION_SIGNATURE)

// Prefer using the scoped macros which automatically end the event (AZ_PROFILE_SCOPE/AZ_PROFILE_FUNCTION)
#   define AZ_PROFILE_BEGIN(category, ...) ::AZ::ProfileScope::BeginRegion(#category, __VA_ARGS__)
#   define AZ_PROFILE_END() ::AZ::ProfileScope::EndRegion()
#endif // AZ_PROFILER_MACRO_DISABLE

#ifndef AZ_PROFILE_INTERVAL_START
#   define AZ_PROFILE_INTERVAL_START(...)
#   define AZ_PROFILE_INTERVAL_START_COLORED(...)
#   define AZ_PROFILE_INTERVAL_END(...)
#   define AZ_PROFILE_INTERVAL_SCOPED(...)
#endif

#ifndef AZ_PROFILE_DATAPOINT
#   define AZ_PROFILE_DATAPOINT(...)
#   define AZ_PROFILE_DATAPOINT_PERCENT(...)
#endif

namespace AZStd
{
    struct thread_id; // forward declare. This is the same type as AZStd::thread::id
}

namespace AZ
{
    class ProfileScope
    {
    public:
        static uint32_t GetSystemID(const char* system);

        template<typename... T>
        static void BeginRegion([[maybe_unused]] const char* system, [[maybe_unused]] char const* eventName, [[maybe_unused]] T const&... args)
        {
            // TODO: Verification that the supplied system name corresponds to a known budget
#if defined(USE_PIX)
            PIXBeginEvent(PIX_COLOR_INDEX(GetSystemID(system) & 0xff), eventName, args...);
#endif
            // TODO: injecting instrumentation for other profilers
        }

        static void EndRegion()
        {
#if defined(USE_PIX)
            PIXEndEvent();
#endif
        }

        template<typename... T>
        ProfileScope(const char* system, char const* eventName, T const&... args)
        {
            BeginRegion(system, eventName, args...);
        }

        ~ProfileScope()
        {
            EndRegion();
        }
    };

    namespace Debug
    {
        class ProfilerSection;
        class ProfilerRegister;
        struct ProfilerThreadData;
        struct ProfilerData;

        /**
         *
         */
        class Profiler
        {
            friend class ProfilerRegister;
            friend struct ProfilerData;
        public:
            /// Max number of threads supported by the profiler.
            static const int m_maxNumberOfThreads = 32;
            /// Max number of systems supported by the profiler. (We can switch this container if needed)
            static const int m_maxNumberOfSystems = 64;

            ~Profiler();

            struct Descriptor
            {
            };

            static bool         Create(const Descriptor& desc = Descriptor());
            static void         Destroy();
            static bool         IsReady()   { return s_instance != NULL; }
            static Profiler&    Instance()  { return *s_instance; }
            static u64          GetId()     { return s_id; }

            /// Increment the use count.
            static void         AddReference();
            /// Release the use count if 0 a Destroy will be called automatically.
            static void         ReleaseReference();

            void                ActivateSystem(const char* systemName);
            void                DeactivateSystem(const char* systemName);
            bool                IsSystemActive(const char* systemName) const;
            bool                IsSystemActive(AZ::u32 systemId) const;
            int                 GetNumberOfSystems() const;
            const char*         GetSystemName(int index) const;
            const char*         GetSystemName(AZ::u32 systemId) const;

            /** Callback to read a single register. Make sure you read the data as fast as possible. Don't compute inside the callback
             * it will lock and hold all the registers that we process. The best is just to read the value and push it into a
             * history buffer.
             */
            typedef AZStd::function<bool (const ProfilerRegister&, const AZStd::thread_id&)> ReadProfileRegisterCB;
            /**
             * Read register values, make sure the code here is fast and efficient as we are holding a lock.
             * provide a callback that will be called for each register.
             * You can choose to filter your values by thread or a system. Use this filter only to narrow you samples. Don't
             * use it for multiple calls to sort your counters, use the history data.
             * It addition keep in mind that you can run this function is parallel, as we only read the values.
             */
            void                ReadRegisterValues(const ReadProfileRegisterCB& callback, AZ::u32 systemFilter = 0, const AZStd::thread_id* threadFilter = NULL) const;
            /**
             * This is slow operation that will cause contention try to avoid using it. A better way will be each frame instead of reset to read and store
             * register values (this is good for history too) and make the difference that way.
             */
            void                ResetRegisters();

            /// You can remove thread data ONLY IF YOU ARE SURE THIS THREAD IS NO LONGER ACTIVE! This will work only is specific cases.
            void                RemoveThreadData(AZStd::thread_id id);

        private:
            /// Register a new system in the profiler. Make sure the proper locks are LOCKED when calling this function (m_threadDataMutex)
            bool                RegisterSystem(AZ::u32 systemId, const char* name, bool isActive);
            /// Unregister a system. Make sure the proper locks are LOCKED when calling this function (m_threadDataMutex)
            bool                UnregisterSystem(AZ::u32 systemId);
            /// Sets the system active/inactive state. Make sure the proper locks are LOCKED when calling this function (m_threadDataMutex)
            bool                SetSystemState(AZ::u32 systemId, bool isActive);

            Profiler(const Descriptor& desc);
            Profiler& operator=(const Profiler&);

            ProfilerData*       m_data;     ///< Hidden data to reduce the number of header files included;
            static Profiler*    s_instance; ///< The only instance of the profiler.
            static u64          s_id;       ///< Profiler unique (over time) id (don't use the pointer as it might be reused).
            static int          s_useCount;
        };

        /**
         * A profiler "virtual" register that contains data about a certain place in the code.
         */
        class ProfilerRegister
        {
            friend class Profiler;
        public:
            ProfilerRegister()
            {}

            enum Type
            {
                PRT_TIME = 0,       ///< Time (members m_time,m_childrenTime,m_calls, m_childrenCalls and m_lastParant are used) register.
                PRT_VALUE,          ///< Value register
            };

            /// Time register data.
            struct TimeData
            {
                AZ::u64             m_time;         ///< Total inclusive time current and children in microseconds.
                AZ::u64             m_childrenTime; ///< Time taken by child profilers in microseconds.
                AZ::s64             m_calls;        ///< Number of calls for this register.
                AZ::s64             m_childrenCalls;///< Number of children calls.
                ProfilerRegister*   m_lastParent;   ///< Pointer to the last parent register.

                static AZStd::chrono::microseconds s_startStopOverheadPer1000Calls; ///< Static constant representing a standard start stop overhead per 1000 calls. You can use this to adjust timings.
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

            static ProfilerRegister*        TimerCreateAndStart(const char* systemName, const char* name, ProfilerSection* section, const char* function, int line);
            static ProfilerRegister*        ValueCreate(const char* systemName, const char* name, const char* function, int line);
            void                            TimerStart(ProfilerSection* section);

            void                            ValueSet(const AZ::s64& v1);
            void                            ValueSet(const AZ::s64& v1, const AZ::s64& v2);
            void                            ValueSet(const AZ::s64& v1, const AZ::s64& v2, const AZ::s64& v3);
            void                            ValueSet(const AZ::s64& v1, const AZ::s64& v2, const AZ::s64& v3, const AZ::s64& v4);
            void                            ValueSet(const AZ::s64& v1, const AZ::s64& v2, const AZ::s64& v3, const AZ::s64& v4, const AZ::s64& v5);

            void                            ValueAdd(const AZ::s64& v1);
            void                            ValueAdd(const AZ::s64& v1, const AZ::s64& v2);
            void                            ValueAdd(const AZ::s64& v1, const AZ::s64& v2, const AZ::s64& v3);
            void                            ValueAdd(const AZ::s64& v1, const AZ::s64& v2, const AZ::s64& v3, const AZ::s64& v4);
            void                            ValueAdd(const AZ::s64& v1, const AZ::s64& v2, const AZ::s64& v3, const AZ::s64& v4, const AZ::s64& v5);

            // Dynamic register data
            union
            {
                TimeData                    m_timeData;
                ValuesData                  m_userValues;
            };

            // Static value data.
            const char*                     m_name;         ///< Name of the profiler register.
            const char*                     m_function;     ///< Function name in the code.
            int                             m_line;         ///< Line number if the code.
            AZ::u32                         m_systemId;     ///< ID of the system this profiler belongs to.
            unsigned char                   m_type : 7;     ///< Register type
            unsigned char                   m_isActive : 1; ///< Flag if the profiler is active.

        private:
            friend class ProfilerSection;

            static ProfilerRegister* CreateRegister(const char* systemName, const char* name, const char* function, int line, ProfilerRegister::Type type);

            /// Compute static start/stop overhead approximation. You can call this periodically (or not) to update the overhead.
            static void                     TimerComputeStartStopOverhead();

            void                            TimerStop();

            void                            Reset();

            ProfilerRegister*               GetValueRegisterForThisThread();

            ProfilerThreadData*             m_threadData;   ///< Pointer to this entry thread data.
        };

        /**
         * Scoped stop register count on destruction.
         */
        class ProfilerSection
        {
            friend class ProfilerRegister;
        public:
            ProfilerSection()
                : m_register(nullptr)
                , m_profilerId(AZ::Debug::Profiler::GetId())
                , m_childTime(0)
                , m_childCalls(0)
            {}

            ~ProfilerSection()
            {
                // If we have a valid register and the profiler did not change while we were active stop the register.
                if (m_register && m_profilerId == AZ::Debug::Profiler::GetId())
                {
                    m_register->TimerStop();
                }
            }

            void Stop()
            {
                // If we have a valid register and the profiler did not change while we were active stop the register.
                if (m_register && m_profilerId == AZ::Debug::Profiler::GetId())
                {
                    m_register->TimerStop();
                }
                m_register = nullptr;
            }
        private:
            ProfilerRegister*                           m_register;     ///< Pointer to the owning profiler register.
            u64                                         m_profilerId;   ///< Id of the profiler when we started this section.
            AZStd::chrono::system_clock::time_point     m_start;        ///< Start mark.
            AZStd::chrono::microseconds                 m_childTime;    ///< Time spent in child profilers.
            int                                         m_childCalls;   ///< Number of children calls.
        };

        AZ_FORCE_INLINE ProfilerRegister*   ProfilerRegister::GetValueRegisterForThisThread()
        {
            return this;
        }

        AZ_FORCE_INLINE void    ProfilerRegister::ValueSet(const AZ::s64& v1)
        {
            ProfilerRegister* reg = GetValueRegisterForThisThread();
            reg->m_userValues.m_value1 = v1;
        }
        AZ_FORCE_INLINE void    ProfilerRegister::ValueSet(const AZ::s64& v1, const AZ::s64& v2)
        {
            ProfilerRegister* reg = GetValueRegisterForThisThread();
            reg->m_userValues.m_value1 = v1;
            reg->m_userValues.m_value2 = v2;
        }
        AZ_FORCE_INLINE void    ProfilerRegister::ValueSet(const AZ::s64& v1, const AZ::s64& v2, const AZ::s64& v3)
        {
            ProfilerRegister* reg = GetValueRegisterForThisThread();
            reg->m_userValues.m_value1 = v1;
            reg->m_userValues.m_value2 = v2;
            reg->m_userValues.m_value3 = v3;
        }
        AZ_FORCE_INLINE void    ProfilerRegister::ValueSet(const AZ::s64& v1, const AZ::s64& v2, const AZ::s64& v3, const AZ::s64& v4)
        {
            ProfilerRegister* reg = GetValueRegisterForThisThread();
            reg->m_userValues.m_value1 = v1;
            reg->m_userValues.m_value2 = v2;
            reg->m_userValues.m_value3 = v3;
            reg->m_userValues.m_value4 = v4;
        }
        AZ_FORCE_INLINE void    ProfilerRegister::ValueSet(const AZ::s64& v1, const AZ::s64& v2, const AZ::s64& v3, const AZ::s64& v4, const AZ::s64& v5)
        {
            ProfilerRegister* reg = GetValueRegisterForThisThread();
            reg->m_userValues.m_value1 = v1;
            reg->m_userValues.m_value2 = v2;
            reg->m_userValues.m_value3 = v3;
            reg->m_userValues.m_value4 = v4;
            reg->m_userValues.m_value5 = v5;
        }
        AZ_FORCE_INLINE void    ProfilerRegister::ValueAdd(const AZ::s64& v1)
        {
            ProfilerRegister* reg = GetValueRegisterForThisThread();
            reg->m_userValues.m_value1 += v1;
        }
        AZ_FORCE_INLINE void    ProfilerRegister::ValueAdd(const AZ::s64& v1, const AZ::s64& v2)
        {
            ProfilerRegister* reg = GetValueRegisterForThisThread();
            reg->m_userValues.m_value1 += v1;
            reg->m_userValues.m_value2 += v2;
        }
        AZ_FORCE_INLINE void    ProfilerRegister::ValueAdd(const AZ::s64& v1, const AZ::s64& v2, const AZ::s64& v3)
        {
            ProfilerRegister* reg = GetValueRegisterForThisThread();
            reg->m_userValues.m_value1 += v1;
            reg->m_userValues.m_value2 += v2;
            reg->m_userValues.m_value3 += v3;
        }
        AZ_FORCE_INLINE void    ProfilerRegister::ValueAdd(const AZ::s64& v1, const AZ::s64& v2, const AZ::s64& v3, const AZ::s64& v4)
        {
            ProfilerRegister* reg = GetValueRegisterForThisThread();
            reg->m_userValues.m_value1 += v1;
            reg->m_userValues.m_value2 += v2;
            reg->m_userValues.m_value3 += v3;
            reg->m_userValues.m_value4 += v4;
        }
        AZ_FORCE_INLINE void    ProfilerRegister::ValueAdd(const AZ::s64& v1, const AZ::s64& v2, const AZ::s64& v3, const AZ::s64& v4, const AZ::s64& v5)
        {
            ProfilerRegister* reg = GetValueRegisterForThisThread();
            reg->m_userValues.m_value1 += v1;
            reg->m_userValues.m_value2 += v2;
            reg->m_userValues.m_value3 += v3;
            reg->m_userValues.m_value4 += v4;
            reg->m_userValues.m_value5 += v5;
        }
    } // namespace Debug

    namespace Internal
    {
        struct RegisterData
        {
            AZ::Debug::ProfilerRegister* m_register;    ///< Pointer to the register data.
            AZ::u64 m_profilerId;               ///< Profiler ID which create the \ref register data.
        };
    }
} // namespace AZ

#ifdef USE_PIX
// The pix3 header unfortunately brings in other Windows macros we need to undef
#undef DeleteFile
#undef LoadImage
#undef GetCurrentTime
#endif
