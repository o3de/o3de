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

        AZStd::shared_ptr<TerrainJobContext> CreateNewTerrainJobContext() override;

        void ProcessHeightsFromListAsync(const AZStd::span<AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        void ProcessNormalsFromListAsync(const AZStd::span<AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        void ProcessSurfaceWeightsFromListAsync(const AZStd::span<AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        void ProcessSurfacePointsFromListAsync(const AZStd::span<AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        void ProcessHeightsFromListOfVector2Async(const AZStd::span<AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        void ProcessNormalsFromListOfVector2Async(const AZStd::span<AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        void ProcessSurfaceWeightsFromListOfVector2Async(const AZStd::span<AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        void ProcessSurfacePointsFromListOfVector2Async(const AZStd::span<AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampleFilter = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;

    private:
        template<typename PerSurfacePointFunctionType, typename VectorType>
        void ProcessFromListAsync(
            PerSurfacePointFunctionType perSurfacePointFunction,
            const AZStd::span<VectorType>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
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
        AZ::JobCancelGroup m_defaultTerrainJobCancelGroup;
        AZStd::unique_ptr<TerrainJobContext> m_defaultTerrainJobContext = nullptr;
    };

    template<typename PerSurfacePointFunctionType, typename VectorType>
    inline void TerrainSystem::ProcessFromListAsync(PerSurfacePointFunctionType perSurfacePointFunction,
        const AZStd::span<VectorType>& inPositions,
        AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
        Sampler sampleFilter,
        AZStd::shared_ptr<ProcessAsyncParams> params) const
    {
        if (!perPositionCallback)
        {
            return;
        }

        // Determine the number of jobs to split the work into based on:
        // 1. The number of available worker threads.
        // 2. The desired number of jobs as passed in.
        // 3. The number of positions being processed.
        //
        // Note: We are currently restricting the number of worker threads to one
        // because splitting the work over multiple threads causes contention when
        // locking various mutexes, resulting in slower overall wall time for async
        // requests split over multiple threads vs one where all the work is done on
        // a single thread. The latter is still preferable over a regular synchronous
        // call because it is just as quick and prevents the main thread from blocking.
        // This should be reverted once the mutex contention issues have been addressed,
        // so that async calls automatically split the work between available job manager
        // worker threads, unless the ProcessAsyncParams specifiy a desired number of jobs.
        const int32_t numWorkerThreads = 1; // m_terrainJobManager->GetNumWorkerThreads();
        const int32_t numJobsDesired = params ? params->m_desiredNumberOfJobs : -1;
        const int32_t numJobsMax = (numJobsDesired > 0) ? AZStd::min(numWorkerThreads, numJobsDesired) : numWorkerThreads;
        const int32_t numPositionsToProcess = static_cast<int32_t>(inPositions.size());
        const int32_t numJobs = AZStd::min(numJobsMax, numPositionsToProcess);
        if (numJobs <= 0)
        {
            AZ_Warning("TerrainSystem", false, "No positions to process.");
            return;
        }

        // Determine the desired terrain job context so the jobs can be cancelled.
        TerrainJobContext* jobContext = (params && params->m_terrainJobContext) ?
                                        params->m_terrainJobContext.get() :
                                        m_defaultTerrainJobContext.get();

        // Split the work across multiple jobs.
        const int32_t numPositionsPerJob = numPositionsToProcess / numJobs;
        AZStd::shared_ptr<AZStd::atomic_int> numJobCompletionsRemaining = AZStd::make_shared<AZStd::atomic_int>(numJobs);
        for (int32_t i = 0; i < numJobs; ++i)
        {
            // If the number of positions can't be divided evenly by the number of jobs,
            // ensure we still process the remaining positions along with the final job.
            const size_t subSpanOffset = i * numPositionsPerJob;
            const size_t subSpanCount = (i < numJobs - 1) ? numPositionsPerJob : AZStd::dynamic_extent;

            // Define the job function using the sub span of positions to process.
            const AZStd::span<VectorType>& positionsToProcess = inPositions.subspan(subSpanOffset, subSpanCount);
            auto jobFunction = [=]()
            {
                // Process the sub span of positions, decrement the number of completions remaining,
                // and invoke the completion callback if this happens to be the final job completed.
                AzFramework::SurfaceData::SurfacePoint surfacePoint;
                for (const auto& position : positionsToProcess)
                {
                    if (jobContext && jobContext->IsCancelled())
                    {
                        // Break out of the loop if the associated job context has been cancelled.
                        break;
                    }

                    bool terrainExists = false;
                    perSurfacePointFunction(surfacePoint, position, sampleFilter, terrainExists);
                    perPositionCallback(surfacePoint, terrainExists);
                }
                if (--(*numJobCompletionsRemaining) == 0 && params && params->m_completionCallback)
                {
                    params->m_completionCallback(params->m_terrainJobContext);
                }
            };

            // Create the job and start it immediately.
            AZ::Job* processJob = AZ::CreateJobFunction(jobFunction, true, jobContext);
            processJob->Start();
        }
    }
} // namespace Terrain
