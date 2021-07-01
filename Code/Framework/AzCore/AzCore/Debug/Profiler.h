/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_PROFILER_H
#define AZCORE_PROFILER_H 1

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/function/function_fwd.h>

namespace AZ
{
    namespace Debug
    {
        using ProfileCategoryPrimitiveType = AZ::u64;

        /**
        * Profiling categories consumed by AZ_PROFILE_FUNCTION and AZ_PROFILE_SCOPE variants for profile filtering
        */
        enum class ProfileCategory : ProfileCategoryPrimitiveType
        {
            // These initial categories match up with the legacy EProfiledSubsystem categories
            Any = 0,
            Renderer,
            ThreeDEngine,
            Particle,
            AI,
            Animation,
            Movie,
            Entity,
            Font,
            Network,
            Physics,
            Script,
            ScriptCFunc,
            Audio,
            Editor,
            System,
            Action,
            Game,
            Input,
            Sync,

            // Legacy network traffic categories
            LegacyNetworkTrafficReserved,
            LegacyDeviceReserved,

            // must match EProfiledSubsystem::PROFILE_LAST_SUBSYSTEM
            LegacyLast,

            // Bulk category via AZ_TRACE_METHOD
            AzTrace,

            AzCore,
            AzRender,
            AzFramework,
            AzToolsFramework,
            ScriptCanvas,
            LegacyTerrain,
            Terrain,
            Cloth,
            // Add new major categories here (and add names to the parallel position in ProfileCategoryNames) - these categories are enabled by default

            FirstDetailedCategory,
            RendererDetailed = FirstDetailedCategory,
            ThreeDEngineDetailed,
            JobManagerDetailed,

            AzRenderDetailed,
            ClothDetailed,
            // Add new detailed categories here (and add names to the parallel position in ProfileCategoryNames) -- these categories are disabled by default
            
            // Internal reserved categories, not for use with performance events
            FirstReservedCategory,
            MemoryReserved = FirstReservedCategory,
            Global,

            // Must be last
            Count
        };
        static_assert(static_cast<size_t>(ProfileCategory::Count) < (sizeof(ProfileCategoryPrimitiveType) * 8), "The number of profile categories must not exceed the number of bits in ProfileCategoryPrimitiveType");

        /**
        * Parallel array to ProfileCategory as string category names to be used as Driller category names or for debug purposes
        */
        static const char * ProfileCategoryNames[] =
        {
            "Any",
            "Renderer",
            "3DEngine",
            "Particle",
            "AI",
            "Animation",
            "Movie",
            "Entity",
            "Font",
            "Network",
            "Physics",
            "Script",
            "ScriptCFunc",
            "Audio",
            "Editor",
            "System",
            "Action",
            "Game",
            "Input",
            "Sync",

            "LegacyNetworkTrafficReserved",
            "LegacyDeviceReserved",

            "LegacyLast",

            "AzTrace",
            "AzCore",
            "AzRender",
            "AzFramework",
            "AzToolsFramework",
            "ScriptCanvas",
            "LegacyTerrain",
            "Terrain",
            "Cloth",

            "RendererDetailed",
            "3DEngineDetailed",
            "JobManagerDetailed",
            "AzRenderDetailed",
            "ClothDetailed",

            "MemoryReserved",
            "Global"
        };
        static_assert(AZ_ARRAY_SIZE(ProfileCategoryNames) == static_cast<size_t>(ProfileCategory::Count), "ProfileCategory and ProfileCategoryNames size mismatch");
    }
}

// Must be included below ProfileCategory 
#ifdef AZ_PROFILE_TELEMETRY
#   include <RADTelemetry/ProfileTelemetry.h>
#endif

#if defined(AZ_PROFILER_MACRO_DISABLE) // by default we never disable the profiler registers as their overhead should be minimal, you can still do that for your code though.
#   define AZ_PROFILE_TIMER(...)
#   define AZ_PROFILE_TIMER_END(_SectionVariableName)
#   define AZ_PROFILE_VALUE_SET(...)
#   define AZ_PROFILE_VALUE_ADD(...)
#   define AZ_PROFILE_VALUE_SET_NAMED(...)
#   define AZ_PROFILE_VALUE_ADD_NAMED(...)
#else
/// Implementation when we have only 1 param system name
#   define AZ_PROFILE_TIMER_1(_1)     AZ_PROFILE_TIMER_2(_1, nullptr)
/// Implementation when we have 2 params (_1 system name and _2 is name of the "section"/register/profiled section - used for debug)
#   define AZ_PROFILE_TIMER_2(_1, _2)  AZ_PROFILE_TIMER_3(_1, _2, AZ_JOIN(azProfileSection, __LINE__))
/// Implementation when we have all 3 params (system name, section/register name, section variable name)
#   define AZ_PROFILE_TIMER_3(_1, _2, _3)                                                                                                                     \
    AZ::Debug::ProfilerSection _3;                                                                                                                            \
    if (AZ::u64 profilerId = AZ::Debug::Profiler::GetId()) {                                                                                                  \
        static AZ_THREAD_LOCAL AZ::Internal::RegisterData AZ_JOIN(azProfileRegister, __LINE__) = {0, 0};                                                      \
        if (AZ_JOIN(azProfileRegister, __LINE__).m_profilerId != profilerId) {                                                                                \
            AZ_JOIN(azProfileRegister, __LINE__).m_register = AZ::Debug::ProfilerRegister::TimerCreateAndStart(_1, _2, &_3, AZ_FUNCTION_SIGNATURE, __LINE__); \
            AZ_JOIN(azProfileRegister, __LINE__).m_profilerId = profilerId;                                                                                   \
        } else {                                                                                                                                              \
            AZ_JOIN(azProfileRegister, __LINE__).m_register->TimerStart(&_3);                                                                                 \
        }                                                                                                                                                     \
    }

/**
 * Macro to declare a profile section for the current scope { }.
 * format is: AZ_PROFILE_TIMER(const char* systemName, const char* sectionDescription = nullptr , optional sectionName )
 * \param _1 is required and it's 'const char*' of the system name of which system this scope/register belongs to.
 * \param _2 is optional and it's 'const char*' with a name for the "section"/register/profiled section - used as description. If not provided a "Anonymous" will be set.
 * \param _3 is optional unique name for a section C++ variable (so you can stop the SCOPE as you wish). If not provided a default unique name is created.
 */
#   define AZ_PROFILE_TIMER(...)        AZ_MACRO_SPECIALIZE(AZ_PROFILE_TIMER_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

// Optional (USE ONLY IN EXTREME CASES!!!) scope end command for named sections, so you stop the profiler register timing before it goes out of scope.
#   define AZ_PROFILE_TIMER_END(_SectionVariableName)  { _SectionVariableName.Stop(); }

/**
 * Macro to operate on custom values. All values are AZ::s64. You can provide up to 5 values.
 * format is AZ_PROFILE_VALUE_SET/ADD(const char* systemName, const char* valueName,
 * value1, optional value2, optional value3, optional value 4, optional value5, optional registerName (for direct register manipulation for EXPERTS ONLY)).
 * \param _SystemName is required and it's 'const char*' of the system name of which system this scope/register belongs to.
 * \param _RegisterName is required and it's 'const char*' with a name for the register - used as description.
 * \param 3 is required and it's AZ::s64, operates on m_value1.
 * \param 4 is optional and it's AZ::s64, operates on m_value2.
 * \param 5 is optional and it's AZ::s64, operates on m_value3.
 * \param 6 is optional and it's AZ::s64, operates on m_value4.
 * \param 7 is optional and it's AZ::s64, operates on m_value5.
 */
#   define AZ_PROFILE_VALUE_SET(_SystemName, _RegisterName, ...)                                                                                                     \
    if (AZ::u64 profilerId = AZ::Debug::Profiler::GetId()) {                                                                                                         \
        static AZ_THREAD_LOCAL AZ::Internal::RegisterData AZ_JOIN(azProfileRegister, __LINE__) = {0, 0};                                                             \
        if (AZ_JOIN(azProfileRegister, __LINE__).m_profilerId != profilerId) {                                                                                       \
            AZ_JOIN(azProfileRegister, __LINE__).m_register = AZ::Debug::ProfilerRegister::ValueCreate(_SystemName, _RegisterName, AZ_FUNCTION_SIGNATURE, __LINE__); \
            AZ_JOIN(azProfileRegister, __LINE__).m_profilerId = profilerId;                                                                                          \
        }                                                                                                                                                            \
        AZ_JOIN(azProfileRegister, __LINE__).m_register->ValueSet(__VA_ARGS__);                                                                                      \
    }

/// Same as AZ_PROFILE_VALUE_SET except is add the values passed in the macro (you can use -(value), to subtract values)
#   define AZ_PROFILE_VALUE_ADD(_SystemName, _RegisterName, ...)                                                                                                     \
    if (AZ::u64 profilerId = AZ::Debug::Profiler::GetId()) {                                                                                                         \
        static AZ_THREAD_LOCAL AZ::Internal::RegisterData AZ_JOIN(azProfileRegister, __LINE__) = {0, 0};                                                             \
        if (AZ_JOIN(azProfileRegister, __LINE__).m_profilerId != profilerId) {                                                                                       \
            AZ_JOIN(azProfileRegister, __LINE__).m_register = AZ::Debug::ProfilerRegister::ValueCreate(_SystemName, _RegisterName, AZ_FUNCTION_SIGNATURE, __LINE__); \
            AZ_JOIN(azProfileRegister, __LINE__).m_profilerId = profilerId;                                                                                          \
        }                                                                                                                                                            \
        AZ_JOIN(azProfileRegister, __LINE__).m_register->ValueAdd(__VA_ARGS__);                                                                                      \
    }

/**
 * Same as AZ_PROFILER_VALUE_SET but with option to access the register by name. (USE ONLY IN EXTREME CASES!!!)
 * \param _RegisterVaribaleName is optional unique name for a register C++ variable so you can manipulate the register.
 */
#   define AZ_PROFILE_VALUE_SET_NAMED(_SystemName, _RegisterName, _RegisterVaribaleName, ...)                                                                        \
    AZ::Debug::ProfilerRegister * _RegisterVaribaleName = nullptr;                                                                                                   \
    if (AZ::u64 profilerId = AZ::Debug::Profiler::GetId()) {                                                                                                         \
        static AZ_THREAD_LOCAL AZ::Internal::RegisterData AZ_JOIN(azProfileRegister, __LINE__) = {0, 0};                                                             \
        if (AZ_JOIN(azProfileRegister, __LINE__).m_profilerId != profilerId) {                                                                                       \
            AZ_JOIN(azProfileRegister, __LINE__).m_register = AZ::Debug::ProfilerRegister::ValueCreate(_SystemName, _RegisterName, AZ_FUNCTION_SIGNATURE, __LINE__); \
            AZ_JOIN(azProfileRegister, __LINE__).m_profilerId = profilerId;                                                                                          \
        }                                                                                                                                                            \
        AZ_JOIN(azProfileRegister, __LINE__).m_register->ValueSet(__VA_ARGS__);                                                                                      \
        _RegisterVaribaleName = AZ_JOIN(azProfileRegister, __LINE__).m_register;                                                                                     \
    }

/// Same as AZ_PROFILE_VALUE_SET_NAMED but add the values to the current. (USE ONLY IN EXTREME CASES!!!)
#   define AZ_PROFILE_VALUE_ADD_NAMED(_SystemName, _RegisterName, _RegisterVaribaleName, ...)                                                                        \
    AZ::Debug::ProfilerRegister * _RegisterVaribaleName = nullptr;                                                                                                   \
    if (AZ::u64 profilerId = AZ::Debug::Profiler::GetId()) {                                                                                                         \
        static AZ_THREAD_LOCAL AZ::Internal::RegisterData AZ_JOIN(azProfileRegister, __LINE__) = {0, 0};                                                             \
        if (AZ_JOIN(azProfileRegister, __LINE__).m_profilerId != profilerId) {                                                                                       \
            AZ_JOIN(azProfileRegister, __LINE__).m_register = AZ::Debug::ProfilerRegister::ValueCreate(_SystemName, _RegisterName, AZ_FUNCTION_SIGNATURE, __LINE__); \
            AZ_JOIN(azProfileRegister, __LINE__).m_profilerId = profilerId;                                                                                          \
        }                                                                                                                                                            \
        AZ_JOIN(azProfileRegister, __LINE__).m_register->ValueAdd(__VA_ARGS__);                                                                                      \
        _RegisterVaribaleName = AZ_JOIN(azProfileRegister, __LINE__).m_register;                                                                                     \
    }

#endif // AZ_PROFILER_MACRO_DISABLE

#ifndef AZ_PROFILE_FUNCTION
// No other profiler has defined the performance markers AZ_PROFILE_SCOPE (and friends), fallback to a Driller implementation
#   define AZ_INTERNAL_PROF_VERIFY_CAT(category) static_assert(category < AZ::Debug::ProfileCategory::Count, "Invalid profile category")
#   define AZ_INTERNAL_PROF_CAT_NAME(category) AZ::Debug::ProfileCategoryNames[static_cast<AZ::u32>(category)]

#   define AZ_PROFILE_FUNCTION(category) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); AZ_PROFILE_TIMER(AZ_INTERNAL_PROF_CAT_NAME(category))
#   define AZ_PROFILE_FUNCTION_STALL(category) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); AZ_PROFILE_TIMER(AZ_INTERNAL_PROF_CAT_NAME(category))
#   define AZ_PROFILE_FUNCTION_IDLE(category) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); AZ_PROFILE_TIMER(AZ_INTERNAL_PROF_CAT_NAME(category))

#   define AZ_PROFILE_SCOPE(category, name) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); AZ_PROFILE_TIMER(AZ_INTERNAL_PROF_CAT_NAME(category)); (void)(name)
#   define AZ_PROFILE_SCOPE_STALL(category, name) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); AZ_PROFILE_TIMER(AZ_INTERNAL_PROF_CAT_NAME(category)); (void)(name)
#   define AZ_PROFILE_SCOPE_IDLE(category, name) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); AZ_PROFILE_TIMER(AZ_INTERNAL_PROF_CAT_NAME(category)); (void)(name)

#   define AZ_PROFILE_SCOPE_DYNAMIC(category, ...) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); AZ_PROFILE_TIMER(AZ_INTERNAL_PROF_CAT_NAME(category))
#   define AZ_PROFILE_SCOPE_STALL_DYNAMIC(category, ...) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); AZ_PROFILE_TIMER(AZ_INTERNAL_PROF_CAT_NAME(category))
#   define AZ_PROFILE_SCOPE_IDLE_DYNAMIC(category, ...) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); AZ_PROFILE_TIMER(AZ_INTERNAL_PROF_CAT_NAME(category))
#endif

#ifndef AZ_PROFILE_EVENT_BEGIN
// No other profiler has defined the performance markers AZ_PROFILE_EVENT_START/END, fallback to a Driller implementation (currently empty)
#   define AZ_PROFILE_EVENT_BEGIN(category, name) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); (void)(name)
#   define AZ_PROFILE_EVENT_END(category) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category)
#endif

#ifndef AZ_PROFILE_INTERVAL_START
// No other profiler has defined the performance markers AZ_PROFILE_INTERVAL_START/END, fallback to a Driller implementation (currently empty)
#   define AZ_INTERNAL_PROF_VERIFY_INTERVAL_ID(id) static_assert(sizeof(id) <= sizeof(AZ::u64), "Interval id must be a unique value no larger than 64-bits")
#   define AZ_PROFILE_INTERVAL_START(category, id, ...) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); AZ_INTERNAL_PROF_VERIFY_INTERVAL_ID(id)
#   define AZ_PROFILE_INTERVAL_START_COLORED(category, id, color, ...) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); (void)(color); AZ_INTERNAL_PROF_VERIFY_INTERVAL_ID(id)
#   define AZ_PROFILE_INTERVAL_END(category, id) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); AZ_INTERNAL_PROF_VERIFY_INTERVAL_ID(id)
#   define AZ_PROFILE_INTERVAL_SCOPED(category, id, ...) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); AZ_INTERNAL_PROF_VERIFY_INTERVAL_ID(id)
#endif

#ifndef AZ_PROFILE_DATAPOINT
// No other profiler has defined the performance markers AZ_PROFILE_DATAPOINT, fallback to a Driller implementation (currently empty)
#define AZ_PROFILE_DATAPOINT(category, value, ...) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); static_cast<double>(value)
#define AZ_PROFILE_DATAPOINT_PERCENT(category, value, ...) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); static_cast<double>(value)
#endif

#ifndef AZ_PROFILE_MEMORY_ALLOC
// No other profiler has defined the performance markers AZ_PROFILE_MEMORY_ALLOC (and friends), fall back to a Driller implementation (currently empty)
#   define AZ_PROFILE_MEMORY_ALLOC(category, address, size, context) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); (void)(context)
#   define AZ_PROFILE_MEMORY_ALLOC_EX(category, filename, lineNumber, address, size, context) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category); (void)(context)
#   define AZ_PROFILE_MEMORY_FREE(category, address) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category)
#   define AZ_PROFILE_MEMORY_FREE_EX(category, filename, lineNumber, address) \
        AZ_INTERNAL_PROF_VERIFY_CAT(category)
#endif

namespace AZStd
{
    struct thread_id; // forward declare. This is the same type as AZStd::thread::id
}

namespace AZ
{
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

#endif // AZCORE_PROFILER_H
#pragma once
