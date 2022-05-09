/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <Vegetation/Ebuses/AreaRequestBus.h>
#include <Vegetation/Ebuses/SystemConfigurationBus.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/parallel/thread.h>
#include <GradientSignal/Ebuses/SectorDataRequestBus.h>
#include <SurfaceData/SurfaceDataSystemNotificationBus.h>
#include <CrySystemBus.h>
#include <ISystem.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>

namespace Vegetation
{
    struct DebugData;

    enum class SnapMode : AZ::u8
    {
        Corner = 0,
        Center
    };

    /**
    * The configuration for managing areas mostly the dimensions of the sectors
    */
    class AreaSystemConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(AreaSystemConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(AreaSystemConfig, "{14CCBE43-52DD-4F56-92A8-2BB011A0F7A2}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        bool operator == (const AreaSystemConfig& other) const
        {
            return m_viewRectangleSize == other.m_viewRectangleSize
                   && m_sectorDensity == other.m_sectorDensity
                   && m_sectorSizeInMeters == other.m_sectorSizeInMeters
                   && m_threadProcessingIntervalMs == other.m_threadProcessingIntervalMs
                   && m_sectorSearchPadding == other.m_sectorSearchPadding
                   && m_sectorPointSnapMode == other.m_sectorPointSnapMode;
        }

        int m_viewRectangleSize = 13;
        int m_sectorDensity = 20;
        int m_sectorSizeInMeters = 16;
        int m_threadProcessingIntervalMs = 500;
        int m_sectorSearchPadding = 0;
        SnapMode m_sectorPointSnapMode = SnapMode::Corner;
    private:
        static const int s_maxViewRectangleSize;
        static const int s_maxSectorDensity;
        static const int s_maxSectorSizeInMeters;

        static const int s_maxInstancesPerMeter;
        static const int64_t s_maxVegetationInstances;

        AZ::Outcome<void, AZStd::string> ValidateViewArea(void* newValue, const AZ::Uuid& valueType);
        AZ::Outcome<void, AZStd::string> ValidateSectorDensity(void* newValue, const AZ::Uuid& valueType);
        AZ::Outcome<void, AZStd::string> ValidateSectorSize(void* newValue, const AZ::Uuid& valueType);
    };

    /**
    * Manages an sectors and claims while the camera scrolls through the 3D world
    */
    class AreaSystemComponent
        : public AZ::Component
        , private AZ::TickBus::Handler
        , private AreaSystemRequestBus::Handler
        , private GradientSignal::SectorDataRequestBus::Handler
        , private SystemConfigurationRequestBus::Handler
        , private CrySystemEventBus::Handler
        , private ISystemEventListener
        , private SurfaceData::SurfaceDataSystemNotificationBus::Handler
        , private AzFramework::Terrain::TerrainDataNotificationBus::Handler
    {
    public:
        friend class EditorAreaSystemComponent;
        AZ_COMPONENT(AreaSystemComponent, "{7CE8E791-6BC6-4C88-8727-A476DE00F9A1}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        AreaSystemComponent(const AreaSystemConfig& configuration);
        AreaSystemComponent() = default;
        ~AreaSystemComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // AreaSystemRequestBus
        void RegisterArea(AZ::EntityId areaId, AZ::u32 layer, AZ::u32 priority, const AZ::Aabb& bounds) override;
        void UnregisterArea(AZ::EntityId areaId) override;
        void RefreshArea(AZ::EntityId areaId, AZ::u32 layer, AZ::u32 priority, const AZ::Aabb& bounds) override;
        void RefreshAllAreas() override;
        void ClearAllAreas() override;
        void MuteArea(AZ::EntityId areaId) override;
        void UnmuteArea(AZ::EntityId areaId) override;
        void EnumerateInstancesInOverlappingSectors(const AZ::Aabb& bounds, AreaSystemEnumerateCallback callback) const override;
        void EnumerateInstancesInAabb(const AZ::Aabb& bounds, AreaSystemEnumerateCallback callback) const override;
        AZStd::size_t GetInstanceCountInAabb(const AZ::Aabb& bounds) const override;
        AZStd::vector<Vegetation::InstanceData> GetInstancesInAabb(const AZ::Aabb& bounds) const override;

        //////////////////////////////////////////////////////////////////////////
        // GradientSignal::SectorDataRequestBus
        void GetPointsPerMeter(float& value) const override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        //////////////////////////////////////////////////////////////////////////
        // SystemConfigurationRequestBus
        void UpdateSystemConfig(const AZ::ComponentConfig* config) override;
        void GetSystemConfig(AZ::ComponentConfig* config) const override;

        //////////////////////////////////////////////////////////////////////////
        // SurfaceData::SurfaceDataSystemNotificationBus
        void OnSurfaceChanged(const AZ::EntityId& entityId, const AZ::Aabb& oldBounds, const AZ::Aabb& newBounds) override;


        ////////////////////////////////////////////////////////////////////////////
        // CrySystemEvents
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override;
        void OnCrySystemShutdown(ISystem& system) override;
        void OnCryEditorBeginLevelExport() override;
        void OnCryEditorEndLevelExport(bool /*success*/) override;
        void OnCryEditorCloseScene() override;

        //////////////////////////////////////////////////////////////////////////
        // ISystemEventListener
        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

        //////////////////////////////////////////////////////////////////////////
        // TerrainDataNotificationBus
        void OnTerrainDataCreateBegin() override;
        void OnTerrainDataDestroyBegin() override;

    private:
        using ClaimContainer = AZStd::unordered_map<ClaimHandle, InstanceData>;
        using ClaimContainerEntry = AZStd::pair<ClaimHandle, InstanceData>;

        using SectorId = AZStd::pair<int, int>;

        // SectorInfo contains basic sector information and the set of "plantable points" in the sector that have been claimed
        struct SectorInfo
        {
            AZ_CLASS_ALLOCATOR(SectorInfo, AZ::SystemAllocator, 0);

            SectorId m_id = {};
            AZ::Aabb m_bounds = {};
            //! Keeps track of points that have been claimed.  This is not cleared at the start of an update pass
            ClaimContainer m_claimedWorldPoints;
            //! Keeps track of previous state of sector while filling to avoid redundant instance destroy/create calls
            ClaimContainer m_claimedWorldPointsBeforeFill;
            ClaimContext m_baseContext;

            int GetSectorX() const { return m_id.first; }
            int GetSectorY() const { return m_id.second; }
        };

        // VegetationAreaInfo contains the basic information we need for tracking which vegetation areas to apply
        // to which sectors, and in which order.
        struct VegetationAreaInfo
        {
            AZ_CLASS_ALLOCATOR(VegetationAreaInfo, AZ::SystemAllocator, 0);

            AZ::EntityId m_id;
            AZ::Aabb m_bounds = {};
            AZ::u32 m_layer = {};
            AZ::u32 m_priority = {};
        };
        using VegetationAreaMap = AZStd::unordered_map<AZ::EntityId, VegetationAreaInfo>;
        using VegetationAreaSet = AZStd::unordered_set<AZ::EntityId>;
        using VegetationAreaVector = AZStd::vector<VegetationAreaInfo>;
        using UnregisteredVegetationAreaMap = AZStd::unordered_map<SectorId, AZStd::unordered_set<AZ::EntityId>>;

        //! Helper class to track whether or not a visible sector is dirty.  Different instances of this
        //! class are used to track different reasons for being dirty.
        //! This is a class instead of just an unordered_set<> so that we can also encapsulate the optimization
        //! of tracking when *all* sectors are dirty.
        class DirtySectors
        {
            public:
                DirtySectors() = default;
                ~DirtySectors() = default;

                void MarkDirty(const SectorId& id);
                void MarkAllDirty();
                bool IsAllDirty() const { return m_allSectorsDirty; }
                bool IsNoneDirty() const { return (!m_allSectorsDirty) && m_dirtySet.empty(); }
                bool IsDirty(const SectorId& id) const;
                void Clear();

            private:
                using DirtySectorSet = AZStd::unordered_set<SectorId>;
                DirtySectorSet m_dirtySet;
                //! Flag when *all* existing sectors are dirty
                bool m_allSectorsDirty = false;
        };


        // ViewRect is a helper struct to manage the "scrolling view rectangle".  This view rectangle controls the
        // set of active spawned vegetation.
        struct ViewRect
        {
            int m_x = 0;
            int m_y = 0;
            int m_width = 0;
            int m_height = 0;
            AZ::Aabb m_viewRectBounds = AZ::Aabb::CreateNull();

            ViewRect() = default;

            ViewRect(int inX, int inY, int inW, int inH, AZ::Aabb viewRectBounds)
                : m_x(inX)
                , m_y(inY)
                , m_width(inW)
                , m_height(inH)
                , m_viewRectBounds(viewRectBounds)
            {
            }

            bool IsInside(const SectorId& sector) const;
            ViewRect Overlap(const ViewRect& b) const;
            bool operator !=(const ViewRect& b);
            bool operator ==(const ViewRect& b);
            size_t GetNumSectors() const;
            int GetMinXSector() const { return m_x; }
            int GetMinYSector() const { return m_y; }
            int GetMaxXSector() const { return m_x + m_width - 1; }
            int GetMaxYSector() const { return m_y + m_height - 1; }
            SectorId GetMinSector() const { return SectorId(GetMinXSector(), GetMinYSector()); }
            SectorId GetMaxSector() const { return SectorId(GetMaxXSector(), GetMaxYSector()); }
            AZ::Aabb GetViewRectBounds() const { return m_viewRectBounds; }
        };

        // Forward declarations, these get defined further down.
        class UpdateContext;
        class PersistentThreadData;

        // Thread-local copies of main state.  We make copies of this to ensure that we can process sectors safely on
        // the vegetation thread while these values potentially get changed on the main thread without needing to wrap
        // all access with mutexes.
        struct CachedMainThreadData
        {
            float m_worldToSector = 0.0f;
            ViewRect m_currViewRect = {};
            int m_sectorSizeInMeters = 0;
            int m_sectorDensity = 0;
            SnapMode m_sectorPointSnapMode = SnapMode::Corner;
        };

        // VegetationThreadTasks is the task queue that's used equally by the main thread and the vegetation thread.
        // The main thread generally queues the tasks, and the vegetation thread processes them.
        // Any data used from this class requires mutexes, atomics, or other thread protection.
        class VegetationThreadTasks
        {
        public:
            void QueueVegetationTask(AZStd::function<void(UpdateContext* context, PersistentThreadData* threadData, VegetationThreadTasks * vegTasks)> func);
            void ProcessVegetationThreadTasks(UpdateContext* context, PersistentThreadData* threadData);
            bool VegetationThreadTasksPending()
            {
                AZStd::lock_guard<decltype(m_vegetationThreadTaskMutex)> lock(m_vegetationThreadTaskMutex);
                return !m_vegetationThreadTasks.empty();
            }

            //! Get sector by 2d veg map coordinates.
            const SectorInfo* GetSector(const SectorId& sectorId) const;
            SectorInfo* GetSector(const SectorId& sectorId);

            SectorInfo* CreateSector(const SectorId& sectorId, int sectorDensity, int sectorSizeInMeters, SnapMode sectorPointSnapMode);
            void UpdateSectorPoints(SectorInfo& sectorInfo, int sectorDensity, int sectorSizeInMeters, SnapMode sectorPointSnapMode);
            void FillSector(SectorInfo& sectorInfo, const VegetationAreaVector& activeAreas);
            void DeleteSector(const SectorId& sectorId);
            void ClearSectors();

            //! Gets the AABB for a sector
            static AZ::Aabb GetSectorBounds(const SectorId& sectorId, int sectorSizeInMeters);

            void FetchDebugData();

            void MarkDirtySectors(const AZ::Aabb& bounds, DirtySectors& dirtySet, float worldToSector, const ViewRect& viewRect);
            void AddUnregisteredVegetationArea(const VegetationAreaInfo& area, float worldToSector, const ViewRect& viewRect);

            //! 2D Array rolling window of sectors that store vegetation objects.
            using SectorRollingWindow = AZStd::unordered_map<SectorId, SectorInfo>;
            mutable AZStd::recursive_mutex m_sectorRollingWindowMutex;
            SectorRollingWindow m_sectorRollingWindow;

        private:
            // claiming logic
            void CreateClaim(SectorInfo& sectorInfo, const ClaimHandle handle, const InstanceData& instanceData);
            ClaimHandle CreateClaimHandle(const SectorInfo& sectorInfo, uint32_t index) const;

            void ReleaseUnusedClaims(SectorInfo& sectorInfo);
            void ReleaseUnregisteredClaims(SectorInfo& sectorInfo);

            //! Creates a new sector
            void UpdateSectorCallbacks(SectorInfo& sectorInfo);

            static void EmptySector(SectorInfo& sectorInfo);

            // Calls the given function on each sector in the box
            template<class Fn>
            static void EnumerateSectorsInAabb(const AZ::Aabb& bounds, float worldToSector, const ViewRect& viewRect, Fn&& fn);


            //! Queued list of vegetation area state update requests.  These get queued on the main thread, and processed
            //! on the vegetation thread.
            mutable AZStd::recursive_mutex m_vegetationThreadTaskMutex;
            using VegetationThreadTaskList = AZStd::list<AZStd::function<void(UpdateContext* context, PersistentThreadData* threadData, VegetationThreadTasks* vegTasks)>>;
            VegetationThreadTaskList m_vegetationThreadTasks;

            //! Map from sectors to areas affecting that sector which have been unregistered and need to have their claims released
            //! Note: This is only updated from the vegetation thread when processing vegetation tasks.
            UnregisteredVegetationAreaMap m_unregisteredVegetationAreaSet;

            //! Cached pointer to the debug data.
            //! Note: This doesn't have an associated mutex because DebugData itself consists purely of atomics
            DebugData* m_debugData = nullptr;
        };

        //! Helper struct to hold the state data used by the vegetation thread.  This contains all the data
        //! that should persist between thread runs, which lets us completely shut down the thread when there's
        //! no work to do.
        //! This also contains the vegetation thread mutex and state variables, which are accessed by both threads
        //! to manage synchronization.
        class PersistentThreadData
        {
        public:
            // This mutex is active the entire time the vegetation thread is running.  Its main purpose
            // is to ensure we don't have component activations / deactivations that occur while the vegetation
            // thread is still processing.
            mutable AZStd::recursive_mutex m_vegetationThreadMutex;

            // Current state of the vegetation thread
            enum class VegetationThreadState
            {
                Stopped,
                Running,
                InterruptRequested
            };
            AZStd::atomic<VegetationThreadState> m_vegetationThreadState{ VegetationThreadState::Stopped };

            // Current state of data synchronization between main thread and vegetation thread
            enum class VegetationDataSyncState
            {
                Synchronized,
                Dirty,
                Updating
            };
            AZStd::atomic<VegetationDataSyncState> m_vegetationDataSyncState{ VegetationDataSyncState::Synchronized };

            //! Reset the states that can get recalculated when the vegetation thread is run.
            //! This does *not* reset the states on registered vegetation area lists, since those only
            //! get filled out once.
            void Init()
            {
                m_activeAreasDirty = true;
                m_activeAreas.clear();
                m_activeAreasInBubble.clear();
                m_dirtySectorContents.Clear();
                m_dirtySectorSurfacePoints.Clear();
            }

            void InterruptVegetationThread();

            // The following state is public because it's accessed by queued vegetation tasks and UpdateContext.  It should
            // eventually be encapsulated a little better.

            //! set of sectors that need their contents refreshed
            DirtySectors m_dirtySectorContents;
            //! set of sectors that need their surface points recalculated (which implies also needing the contents refreshed)
            DirtySectors m_dirtySectorSurfacePoints;

            VegetationAreaMap m_globalVegetationAreaMap;
            VegetationAreaSet m_ignoredVegetationAreaSet;

            //! Determines when to refresh the set of active areas
            bool m_activeAreasDirty = true;

        protected:
            // Only UpdateContext is allowed to directly access the persisted thread state.
            friend class UpdateContext;

            // This is effectively a local variable in UpdateActiveVegetationAreas, but is kept
            // persistent to avoid potentially frequent reallocation.
            VegetationAreaVector m_activeAreas;

            //! The set of active vegetation areas that overlap the current view rectangle
            VegetationAreaVector m_activeAreasInBubble;
        };


        // UpdateContext is the logic that normally runs on the vegetation thread.  It processes all of the logic needed to update and fill
        // any vegetation sectors that are currently within the view rectangle.  Occasionally the logic will be triggered on the main thread
        // in cases such as shutdown when the vegetation thread isn't running any we need to perform the cleanup synchronously.
        class UpdateContext
        {
        public:
            UpdateContext() = default;

            void Run(PersistentThreadData* threadData, VegetationThreadTasks* vegTasks, CachedMainThreadData* cachedMainThreadData);
            void UpdateActiveVegetationAreas(PersistentThreadData* threadData, const ViewRect& viewRect);
            const CachedMainThreadData& GetCachedMainThreadData() { return m_cachedMainThreadData; }

        private:
            bool UpdateSectorWorkLists(PersistentThreadData* threadData, VegetationThreadTasks* vegTasks);
            bool UpdateOneSector(PersistentThreadData* threadData, VegetationThreadTasks* vegTasks);

            enum class UpdateMode
            {
                Create,
                RebuildSurfaceCacheAndFill,
                Fill
            };

            // The sorted work list of sectors to delete.  The list is recreated every time UpdateSectorWorkLists() is run.
            AZStd::vector<SectorId> m_deleteWorkList;

            // The sorted work list of sectors to create / update.  This is incrementally modified when UpdateSectorWorkLists()
            // is run, because any previously-requested updates that are still in view need to be preserved.  They can't simply
            // be recalculated.
            AZStd::vector<AZStd::pair<SectorId, UpdateMode>> m_updateWorkList;

            // Sector counts of the number of expected sectors in the view rectangle vs the number of sectors
            // currently active.  These are used to "load balance" sector deletes and creates so that we don't have
            // too many sectors active at any one point in time.
            size_t m_viewRectSectorCount = 0;

            // Thread-local copy of the main thread's m_cachedMainThreadData.  This way we can read from it on the vegetation
            // thread without requiring mutexes.
            CachedMainThreadData m_cachedMainThreadData;

        };

        bool ApplyPendingConfigChanges();

        void ReleaseData();
        void ReleaseAllClaims();
        void ReleaseWithoutCleanup();

        bool CalculateViewRect();


        //! Get sector id by world coordinates.
        static SectorId GetSectorId(const AZ::Vector3& worldPos, float worldToSector);


        // All of this state data should only get accessed by the main thread.  A subset of this data gets copied
        // into CachedMainThreadData for the vegetation thread to be able to query in a lockless manner.
        AreaSystemConfig m_configuration;
        float m_worldToSector = 0.0f;       //! world to sector scaling ratio.
        ViewRect m_currViewRect = {};
        float m_vegetationThreadTaskTimer = 0.0f;
        ISystem* m_system = nullptr;
        bool m_configDirty = false;
        AreaSystemConfig m_pendingConfigUpdate;

        // The vegetation task queue gets read/written from both threads, and uses atomics + mutexes for synchronization.
        VegetationThreadTasks m_vegTasks;

        // This state should only get read or written from the vegetation thread, except for component initialization.
        PersistentThreadData m_threadData;

        // This state gets written to from the main thread, and gets copied and read from the vegetation thread.
        CachedMainThreadData m_cachedMainThreadData;
    };
}
