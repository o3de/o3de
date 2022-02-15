/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/parallel/condition_variable.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Aabb.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Jobs/JobFunction.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <TerrainRaycast/TerrainRaycastContext.h>
#include <TerrainSystem/TerrainSystemBus.h>

namespace Terrain
{
    struct TerrainLayerPriorityComparator
    {
        bool operator()(const AZ::EntityId& layer1id, const AZ::EntityId& layer2id) const;
    };

    class TerrainSystem
        : public AzFramework::Terrain::TerrainDataRequestBus::Handler
        , private Terrain::TerrainSystemServiceRequestBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        TerrainSystem();
        ~TerrainSystem();

        ///////////////////////////////////////////
        // TerrainSystemServiceRequestBus::Handler Impl
        void Activate() override;
        void Deactivate() override;

        void RegisterArea(AZ::EntityId areaId) override;
        void UnregisterArea(AZ::EntityId areaId) override;
        void RefreshArea(
            AZ::EntityId areaId, AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask changeMask) override;

        ///////////////////////////////////////////
        // TerrainDataRequestBus::Handler Impl
        float GetTerrainHeightQueryResolution() const override;
        void SetTerrainHeightQueryResolution(float queryResolution) override;

        AZ::Aabb GetTerrainAabb() const override;
        void SetTerrainAabb(const AZ::Aabb& worldBounds) override;


        //! Returns terrains height in meters at location x,y.
        //! @terrainExistsPtr: Can be nullptr. If != nullptr then, if there's no terrain at location x,y or location x,y is inside a terrain
        //! HOLE then *terrainExistsPtr will become false,
        //!                  otherwise *terrainExistsPtr will become true.
        float GetHeight(const AZ::Vector3& position, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;
        float GetHeightFromVector2(const AZ::Vector2& position, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;
        float GetHeightFromFloats(float x, float y, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;

        //! Given an XY coordinate, return the max surface type and weight.
        //! @terrainExists: Can be nullptr. If != nullptr then, if there's no terrain at location x,y or location x,y is inside a terrain
        //! HOLE then *terrainExistsPtr will be set to false,
        //!                  otherwise *terrainExistsPtr will be set to true.
        AzFramework::SurfaceData::SurfaceTagWeight GetMaxSurfaceWeight(
            const AZ::Vector3& position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;
        AzFramework::SurfaceData::SurfaceTagWeight GetMaxSurfaceWeightFromVector2(
            const AZ::Vector2& inPosition, Sampler sampleFilter = Sampler::DEFAULT, bool* terrainExistsPtr = nullptr) const override;
        AzFramework::SurfaceData::SurfaceTagWeight GetMaxSurfaceWeightFromFloats(
            const float x, const float y, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;

        void GetSurfaceWeights(
            const AZ::Vector3& inPosition,
            AzFramework::SurfaceData::SurfaceTagWeightList& outSurfaceWeights,
            Sampler sampleFilter = Sampler::DEFAULT,
            bool* terrainExistsPtr = nullptr) const override;
        void GetSurfaceWeightsFromVector2(
            const AZ::Vector2& inPosition,
            AzFramework::SurfaceData::SurfaceTagWeightList& outSurfaceWeights,
            Sampler sampleFilter = Sampler::DEFAULT,
            bool* terrainExistsPtr = nullptr) const override;
        void GetSurfaceWeightsFromFloats(
            float x,
            float y,
            AzFramework::SurfaceData::SurfaceTagWeightList& outSurfaceWeights,
            Sampler sampleFilter = Sampler::DEFAULT,
            bool* terrainExistsPtr = nullptr) const override;

        //! Convenience function for  low level systems that can't do a reverse lookup from Crc to string. Everyone else should use
        //! GetMaxSurfaceWeight or GetMaxSurfaceWeightFromFloats. Not available in the behavior context. Returns nullptr if the position is
        //! inside a hole or outside of the terrain boundaries.
        const char* GetMaxSurfaceName(
            const AZ::Vector3& position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;

        //! Returns true if there's a hole at location x,y.
        //! Also returns true if there's no terrain data at location x,y.
        bool GetIsHole(const AZ::Vector3& position, Sampler sampleFilter = Sampler::BILINEAR) const override;
        bool GetIsHoleFromVector2(const AZ::Vector2& position, Sampler sampleFilter = Sampler::BILINEAR) const override;
        bool GetIsHoleFromFloats(float x, float y, Sampler sampleFilter = Sampler::BILINEAR) const override;

        // Given an XY coordinate, return the surface normal.
        //! @terrainExists: Can be nullptr. If != nullptr then, if there's no terrain at location x,y or location x,y is inside a terrain
        //! HOLE then *terrainExistsPtr will be set to false,
        //!                  otherwise *terrainExistsPtr will be set to true.
        AZ::Vector3 GetNormal(
            const AZ::Vector3& position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;
        AZ::Vector3 GetNormalFromVector2(
            const AZ::Vector2& position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;
        AZ::Vector3 GetNormalFromFloats(
            float x, float y, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;

        void GetSurfacePoint(
            const AZ::Vector3& inPosition,
            AzFramework::SurfaceData::SurfacePoint& outSurfacePoint,
            Sampler sampleFilter = Sampler::DEFAULT,
            bool* terrainExistsPtr = nullptr) const override;
        void GetSurfacePointFromVector2(
            const AZ::Vector2& inPosition,
            AzFramework::SurfaceData::SurfacePoint& outSurfacePoint,
            Sampler sampleFilter = Sampler::DEFAULT,
            bool* terrainExistsPtr = nullptr) const override;
        void GetSurfacePointFromFloats(
            float x,
            float y,
            AzFramework::SurfaceData::SurfacePoint& outSurfacePoint,
            Sampler sampleFilter = Sampler::DEFAULT,
            bool* terrainExistsPtr = nullptr) const override;

        //! Given a list of XY coordinates, call the provided callback function with surface data corresponding to each
        //! XY coordinate in the list.
        virtual void ProcessHeightsFromList(const AZStd::span<AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT) const override;
        virtual void ProcessNormalsFromList(const AZStd::span<AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT) const override;
        virtual void ProcessSurfaceWeightsFromList(const AZStd::span<AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT) const override;
        virtual void ProcessSurfacePointsFromList(const AZStd::span<AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT) const override;
        virtual void ProcessHeightsFromListOfVector2(const AZStd::span<AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT) const override;
        virtual void ProcessNormalsFromListOfVector2(const AZStd::span<AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT) const override;
        virtual void ProcessSurfaceWeightsFromListOfVector2(const AZStd::span<AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT) const override;
        virtual void ProcessSurfacePointsFromListOfVector2(const AZStd::span<AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT) const override;

        //! Returns the number of samples for a given region and step size. The first and second
        //! elements of the pair correspond to the X and Y sample counts respectively.
        virtual AZStd::pair<size_t, size_t> GetNumSamplesFromRegion(const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize) const override;

        //! Given a region(aabb) and a step size, call the provided callback function with surface data corresponding to the
        //! coordinates in the region.
        virtual void ProcessHeightsFromRegion(const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT) const override;
        virtual void ProcessNormalsFromRegion(const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT) const override;
        virtual void ProcessSurfaceWeightsFromRegion(const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT) const override;
        virtual void ProcessSurfacePointsFromRegion(const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT) const override;

        AzFramework::EntityContextId GetTerrainRaycastEntityContextId() const override;
        AzFramework::RenderGeometry::RayResult GetClosestIntersection(
            const AzFramework::RenderGeometry::RayRequest& ray) const override;

        AZStd::shared_ptr<TerrainJobContext> ProcessHeightsFromListAsync(
            const AZStd::span<AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessNormalsFromListAsync(
            const AZStd::span<AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessSurfaceWeightsFromListAsync(
            const AZStd::span<AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessSurfacePointsFromListAsync(
            const AZStd::span<AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessHeightsFromListOfVector2Async(
            const AZStd::span<AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessNormalsFromListOfVector2Async(
            const AZStd::span<AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessSurfaceWeightsFromListOfVector2Async(
            const AZStd::span<AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessSurfacePointsFromListOfVector2Async(
            const AZStd::span<AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessHeightsFromRegionAsync(
            const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessNormalsFromRegionAsync(
            const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessSurfaceWeightsFromRegionAsync(
            const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessSurfacePointsFromRegionAsync(
            const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;

    private:
        template<typename SynchronousFunctionType, typename VectorType>
        AZStd::shared_ptr<TerrainJobContext> ProcessFromListAsync(
            SynchronousFunctionType synchronousFunction,
            const AZStd::span<VectorType>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const;

        template<typename SynchronousFunctionType>
        AZStd::shared_ptr<TerrainJobContext> ProcessFromRegionAsync(
            SynchronousFunctionType synchronousFunction,
            const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const;

        void ClampPosition(float x, float y, AZ::Vector2& outPosition, AZ::Vector2& normalizedDelta) const;
        bool InWorldBounds(float x, float y) const;

        AZ::EntityId FindBestAreaEntityAtPosition(float x, float y, AZ::Aabb& bounds) const;
        void GetOrderedSurfaceWeights(
            const float x,
            const float y,
            Sampler sampler,
            AzFramework::SurfaceData::SurfaceTagWeightList& outSurfaceWeights,
            bool* terrainExistsPtr) const;
        float GetHeightSynchronous(float x, float y, Sampler sampler, bool* terrainExistsPtr) const;
        float GetTerrainAreaHeight(float x, float y, bool& terrainExists) const;
        AZ::Vector3 GetNormalSynchronous(float x, float y, Sampler sampler, bool* terrainExistsPtr) const;

        typedef AZStd::function<void(
            const AZStd::span<AZ::Vector3> inPositions,
            AZStd::span<AZ::Vector3> outPositions,
            AZStd::span<bool> outTerrainExists,
            AZStd::span<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeights,
            AZ::EntityId areaId)> BulkQueriesCallback;

        void GetHeightsSynchronous(
            const AZStd::span<AZ::Vector3>& inPositions,
            Sampler sampler, AZStd::span<float> heights,
            AZStd::span<bool> terrainExists) const;
        void GetNormalsSynchronous(
            const AZStd::span<AZ::Vector3>& inPositions,
            Sampler sampler, AZStd::span<AZ::Vector3> normals,
            AZStd::span<bool> terrainExists) const;
        void GetOrderedSurfaceWeightsFromList(
            const AZStd::span<AZ::Vector3>& inPositions, Sampler sampler,
            AZStd::span<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeightsList,
            AZStd::span<bool> terrainExists) const;
        void MakeBulkQueries(
            const AZStd::span<AZ::Vector3> inPositions,
            AZStd::span<AZ::Vector3> outPositions,
            AZStd::span<bool> outTerrainExists,
            AZStd::span<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWieghts,
            BulkQueriesCallback queryCallback) const;
        void GenerateQueryPositions(const AZStd::span<AZ::Vector3>& inPositions, 
            AZStd::vector<AZ::Vector3>& outPositions,
            Sampler sampler) const;
        AZStd::vector<AZ::Vector3> GenerateInputPositionsFromRegion(
            const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize) const;

        // AZ::TickBus::Handler overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        struct TerrainSystemSettings
        {
            AZ::Aabb m_worldBounds;
            float m_heightQueryResolution{ 1.0f };
            bool m_systemActive{ false };
        };

        TerrainSystemSettings m_currentSettings;
        TerrainSystemSettings m_requestedSettings;

        bool m_terrainSettingsDirty = true;
        bool m_terrainHeightDirty = false;
        bool m_terrainSurfacesDirty = false;
        AZ::Aabb m_dirtyRegion;

        // Cached data for each terrain area to use when looking up terrain data.
        struct TerrainAreaData
        {
            AZ::Aabb m_areaBounds{ AZ::Aabb::CreateNull() };
            bool m_useGroundPlane{ false };
        };

        mutable AZStd::shared_mutex m_areaMutex;
        AZStd::map<AZ::EntityId, TerrainAreaData, TerrainLayerPriorityComparator> m_registeredAreas;

        mutable TerrainRaycastContext m_terrainRaycastContext;

        AZ::JobManager* m_terrainJobManager = nullptr;
        mutable AZStd::mutex m_activeTerrainJobContextMutex;
        mutable AZStd::condition_variable m_activeTerrainJobContextMutexConditionVariable;
        mutable AZStd::deque<AZStd::shared_ptr<TerrainJobContext>> m_activeTerrainJobContexts;
    };

    template<typename SynchronousFunctionType, typename VectorType>
    inline AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext> TerrainSystem::ProcessFromListAsync(
        SynchronousFunctionType synchronousFunction,
        const AZStd::span<VectorType>& inPositions,
        AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
        Sampler sampleFilter,
        AZStd::shared_ptr<ProcessAsyncParams> params) const
    {
        // Determine the number of jobs to split the work into based on:
        // 1. The number of available worker threads.
        // 2. The desired number of jobs as passed in.
        // 3. The number of positions being processed.
        const int32_t numWorkerThreads = m_terrainJobManager->GetNumWorkerThreads();
        const int32_t numJobsDesired = params ? params->m_desiredNumberOfJobs : ProcessAsyncParams::NumJobsDefault;
        const int32_t numJobsMax = (numJobsDesired > 0) ? AZStd::min(numWorkerThreads, numJobsDesired) : numWorkerThreads;
        const int32_t numPositionsToProcess = static_cast<int32_t>(inPositions.size());
        const int32_t minPositionsPerJob = params && (params->m_desiredNumberOfJobs > 0) ? params->m_desiredNumberOfJobs : ProcessAsyncParams::MinPositionsPerJobDefault;
        const int32_t numJobs = AZStd::min(numJobsMax, numPositionsToProcess / minPositionsPerJob);
        if (numJobs <= 0)
        {
            AZ_Warning("TerrainSystem", false, "No positions to process.");
            return nullptr;
        }

        // Create a terrain job context, track it, and split the work across multiple jobs.
        AZStd::shared_ptr<TerrainJobContext> jobContext = AZStd::make_shared<TerrainJobContext>(*m_terrainJobManager, numJobs);
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_activeTerrainJobContextMutex);
            m_activeTerrainJobContexts.push_back(jobContext);
        }
        const int32_t numPositionsPerJob = numPositionsToProcess / numJobs;
        for (int32_t i = 0; i < numJobs; ++i)
        {
            // If the number of positions can't be divided evenly by the number of jobs,
            // ensure we still process the remaining positions along with the final job.
            const size_t subSpanOffset = i * numPositionsPerJob;
            const size_t subSpanCount = (i < numJobs - 1) ? numPositionsPerJob : AZStd::dynamic_extent;

            // Define the job function using the sub span of positions to process.
            const AZStd::span<VectorType>& positionsToProcess = inPositions.subspan(subSpanOffset, subSpanCount);
            auto jobFunction = [this, synchronousFunction, positionsToProcess, perPositionCallback, sampleFilter, jobContext, params]()
            {
                // Process the sub span of positions, unless the associated job context has been cancelled.
                if (!jobContext->IsCancelled())
                {
                    synchronousFunction(positionsToProcess, perPositionCallback, sampleFilter);
                }

                // Decrement the number of completions remaining, invoke the completion callback if this happens
                // to be the final job completed, and remove this TerrainJobContext from the list of active ones.
                const bool wasLastJobCompleted = jobContext->OnJobCompleted();
                if (wasLastJobCompleted)
                {
                    if (params && params->m_completionCallback)
                    {
                        params->m_completionCallback(jobContext);
                    }

                    {
                        AZStd::unique_lock<AZStd::mutex> lock(m_activeTerrainJobContextMutex);
                        m_activeTerrainJobContexts.erase(AZStd::find(m_activeTerrainJobContexts.begin(),
                                                                     m_activeTerrainJobContexts.end(),
                                                                     jobContext));
                        m_activeTerrainJobContextMutexConditionVariable.notify_one();
                    }
                }
            };

            // Create the job and start it immediately.
            AZ::Job* processJob = AZ::CreateJobFunction(jobFunction, true, jobContext.get());
            processJob->Start();
        }

        return jobContext;
    }

    template<typename SynchronousFunctionType>
    inline AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext> TerrainSystem::ProcessFromRegionAsync(
        SynchronousFunctionType synchronousFunction,
        const AZ::Aabb& inRegion,
        const AZ::Vector2& stepSize,
        AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
        Sampler sampleFilter,
        AZStd::shared_ptr<ProcessAsyncParams> params) const
    {
        // ToDo: Determine the number of jobs to split the work into based on:
        // 1. The number of available worker threads.
        // 2. The desired number of jobs as passed in.
        // 3. The size of the area being processed.
        //
        // Note: We are currently restricting the number of worker threads to one
        // because splitting the work over multiple threads causes contention when
        // locking various mutexes, resulting in slower overall wall time for async
        // requests split over multiple threads vs one where all the work is done on
        // a single thread. The latter is still preferable over a regular synchronous
        // call because it is just as quick and prevents the main thread from blocking.
        // Once the mutex contention issues have been addressed, we should come up with
        // an algorithm to break up 'inRegion' into sub-regions (or lists of positions?)
        // so that async calls automatically split the work between available job manager
        // worker threads, unless the ProcessAsyncParams specifiy a desired number of jobs.
        const int32_t numWorkerThreads = m_terrainJobManager->GetNumWorkerThreads();
        const int32_t numJobsDesired = params ? params->m_desiredNumberOfJobs : ProcessAsyncParams::NumJobsDefault;
        int32_t numJobs = (numJobsDesired > 0) ? AZStd::min(numWorkerThreads, numJobsDesired) : numWorkerThreads;
        if (numJobs != 1)
        {
            // Temp until we figure out how to break up the region.
            AZ_Warning("TerrainSystem", false, "We don't yet support breaking up regions.");
            numJobs = 1;
        }

        // Create a terrain job context and split the work across multiple jobs.
        AZStd::shared_ptr<TerrainJobContext> jobContext = AZStd::make_shared<TerrainJobContext>(*m_terrainJobManager, numJobs);
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_activeTerrainJobContextMutex);
            m_activeTerrainJobContexts.push_back(jobContext);
        }
        for (int32_t i = 0; i < numJobs; ++i)
        {
            // Define the job function using the sub region of positions to process.
            const AZ::Aabb& subRegion = inRegion; // ToDo: Figure out how to break up the region.
            auto jobFunction = [this, synchronousFunction, subRegion, stepSize, perPositionCallback, sampleFilter, jobContext, params]()
            {
                // Process the sub region of positions, unless the associated job context has been cancelled.
                if (!jobContext->IsCancelled())
                {
                    synchronousFunction(subRegion, stepSize, perPositionCallback, sampleFilter);
                }

                // Decrement the number of completions remaining, invoke the completion callback if this happens
                // to be the final job completed, and remove this TerrainJobContext from the list of active ones.
                const bool wasLastJobCompleted = jobContext->OnJobCompleted();
                if (wasLastJobCompleted)
                {
                    if (params && params->m_completionCallback)
                    {
                        params->m_completionCallback(jobContext);
                    }

                    {
                        AZStd::unique_lock<AZStd::mutex> lock(m_activeTerrainJobContextMutex);
                        m_activeTerrainJobContexts.erase(AZStd::find(m_activeTerrainJobContexts.begin(),
                                                                     m_activeTerrainJobContexts.end(),
                                                                     jobContext));
                        m_activeTerrainJobContextMutexConditionVariable.notify_one();
                    }
                }
            };

            // Create the job and start it immediately.
            AZ::Job* processJob = AZ::CreateJobFunction(jobFunction, true, jobContext.get());
            processJob->Start();
        }

        return jobContext;
    }
} // namespace Terrain
