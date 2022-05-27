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

AZ_DECLARE_BUDGET(Terrain);

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
        float GetTerrainSurfaceDataQueryResolution() const override;
        void SetTerrainSurfaceDataQueryResolution(float queryResolution) override;

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
            const AZ::Vector3& position, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;
        AzFramework::SurfaceData::SurfaceTagWeight GetMaxSurfaceWeightFromVector2(
            const AZ::Vector2& inPosition, Sampler sampler = Sampler::DEFAULT, bool* terrainExistsPtr = nullptr) const override;
        AzFramework::SurfaceData::SurfaceTagWeight GetMaxSurfaceWeightFromFloats(
            const float x, const float y, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;

        void GetSurfaceWeights(
            const AZ::Vector3& inPosition,
            AzFramework::SurfaceData::SurfaceTagWeightList& outSurfaceWeights,
            Sampler sampler = Sampler::DEFAULT,
            bool* terrainExistsPtr = nullptr) const override;
        void GetSurfaceWeightsFromVector2(
            const AZ::Vector2& inPosition,
            AzFramework::SurfaceData::SurfaceTagWeightList& outSurfaceWeights,
            Sampler sampler = Sampler::DEFAULT,
            bool* terrainExistsPtr = nullptr) const override;
        void GetSurfaceWeightsFromFloats(
            float x,
            float y,
            AzFramework::SurfaceData::SurfaceTagWeightList& outSurfaceWeights,
            Sampler sampler = Sampler::DEFAULT,
            bool* terrainExistsPtr = nullptr) const override;

        //! Convenience function for  low level systems that can't do a reverse lookup from Crc to string. Everyone else should use
        //! GetMaxSurfaceWeight or GetMaxSurfaceWeightFromFloats. Not available in the behavior context. Returns nullptr if the position is
        //! inside a hole or outside of the terrain boundaries.
        const char* GetMaxSurfaceName(
            const AZ::Vector3& position, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;

        //! Returns true if there's a hole at location x,y.
        //! Also returns true if there's no terrain data at location x,y.
        bool GetIsHole(const AZ::Vector3& position, Sampler sampler = Sampler::BILINEAR) const override;
        bool GetIsHoleFromVector2(const AZ::Vector2& position, Sampler sampler = Sampler::BILINEAR) const override;
        bool GetIsHoleFromFloats(float x, float y, Sampler sampler = Sampler::BILINEAR) const override;

        // Given an XY coordinate, return the surface normal.
        //! @terrainExists: Can be nullptr. If != nullptr then, if there's no terrain at location x,y or location x,y is inside a terrain
        //! HOLE then *terrainExistsPtr will be set to false,
        //!                  otherwise *terrainExistsPtr will be set to true.
        AZ::Vector3 GetNormal(
            const AZ::Vector3& position, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;
        AZ::Vector3 GetNormalFromVector2(
            const AZ::Vector2& position, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;
        AZ::Vector3 GetNormalFromFloats(
            float x, float y, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;

        void GetSurfacePoint(
            const AZ::Vector3& inPosition,
            AzFramework::SurfaceData::SurfacePoint& outSurfacePoint,
            Sampler sampler = Sampler::DEFAULT,
            bool* terrainExistsPtr = nullptr) const override;
        void GetSurfacePointFromVector2(
            const AZ::Vector2& inPosition,
            AzFramework::SurfaceData::SurfacePoint& outSurfacePoint,
            Sampler sampler = Sampler::DEFAULT,
            bool* terrainExistsPtr = nullptr) const override;
        void GetSurfacePointFromFloats(
            float x,
            float y,
            AzFramework::SurfaceData::SurfacePoint& outSurfacePoint,
            Sampler sampler = Sampler::DEFAULT,
            bool* terrainExistsPtr = nullptr) const override;

        //! Given a list of XY coordinates, call the provided callback function with surface data corresponding to each
        //! XY coordinate in the list.
        void ProcessHeightsFromList(const AZStd::span<const AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT) const override;
        void ProcessNormalsFromList(const AZStd::span<const AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT) const override;
        void ProcessSurfaceWeightsFromList(const AZStd::span<const AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT) const override;
        void ProcessSurfacePointsFromList(const AZStd::span<const AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT) const override;
        void ProcessHeightsFromListOfVector2(const AZStd::span<const AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT) const override;
        void ProcessNormalsFromListOfVector2(const AZStd::span<const AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT) const override;
        void ProcessSurfaceWeightsFromListOfVector2(const AZStd::span<const AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT) const override;
        void ProcessSurfacePointsFromListOfVector2(const AZStd::span<const AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT) const override;

        //! Returns the number of samples for a given region and step size. The first and second
        //! elements of the pair correspond to the X and Y sample counts respectively.
        AZStd::pair<size_t, size_t> GetNumSamplesFromRegion(const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize, Sampler sampler) const override;

        //! Given a region(aabb) and a step size, call the provided callback function with surface data corresponding to the
        //! coordinates in the region.
        void ProcessHeightsFromRegion(const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT) const override;
        void ProcessNormalsFromRegion(const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT) const override;
        void ProcessSurfaceWeightsFromRegion(const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT) const override;
        void ProcessSurfacePointsFromRegion(const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT) const override;

        AzFramework::EntityContextId GetTerrainRaycastEntityContextId() const override;
        AzFramework::RenderGeometry::RayResult GetClosestIntersection(
            const AzFramework::RenderGeometry::RayRequest& ray) const override;

        AZStd::shared_ptr<TerrainJobContext> ProcessHeightsFromListAsync(
            const AZStd::span<const AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessNormalsFromListAsync(
            const AZStd::span<const AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessSurfaceWeightsFromListAsync(
            const AZStd::span<const AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessSurfacePointsFromListAsync(
            const AZStd::span<const AZ::Vector3>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessHeightsFromListOfVector2Async(
            const AZStd::span<const AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessNormalsFromListOfVector2Async(
            const AZStd::span<const AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessSurfaceWeightsFromListOfVector2Async(
            const AZStd::span<const AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessSurfacePointsFromListOfVector2Async(
            const AZStd::span<const AZ::Vector2>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessHeightsFromRegionAsync(
            const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessNormalsFromRegionAsync(
            const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessSurfaceWeightsFromRegionAsync(
            const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<TerrainJobContext> ProcessSurfacePointsFromRegionAsync(
            const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const override;

    private:
        //! Given a set of async parameters, calculate the max number of jobs that we can use for the async call.
        int32_t CalculateMaxJobs(AZStd::shared_ptr<ProcessAsyncParams> params) const;

        //! Given the number of samples in a region and the desired number of jobs, choose the best subdivision of the region into jobs.
        static void SubdivideRegionForJobs(
            int32_t numSamplesX, int32_t numSamplesY, int32_t maxNumJobs, int32_t minPointsPerJob,
            int32_t& subdivisionsX, int32_t& subdivisionsY);

        template<typename SynchronousFunctionType, typename VectorType>
        AZStd::shared_ptr<TerrainJobContext> ProcessFromListAsync(
            SynchronousFunctionType synchronousFunction,
            const AZStd::span<const VectorType>& inPositions,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const;

        template<typename SynchronousFunctionType>
        AZStd::shared_ptr<TerrainJobContext> ProcessFromRegionAsync(
            SynchronousFunctionType synchronousFunction,
            const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const;

        void ClampPosition(float x, float y, AZ::Vector2& outPosition, AZ::Vector2& normalizedDelta) const;
        bool InWorldBounds(float x, float y) const;

        AZ::EntityId FindBestAreaEntityAtPosition(const AZ::Vector3& position, AZ::Aabb& bounds) const;
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
            const AZStd::span<const AZ::Vector3> inPositions,
            AZStd::span<AZ::Vector3> outPositions,
            AZStd::span<bool> outTerrainExists,
            AZStd::span<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeights,
            AZ::EntityId areaId)> BulkQueriesCallback;

        void GetHeightsSynchronous(
            const AZStd::span<const AZ::Vector3>& inPositions,
            Sampler sampler, AZStd::span<float> heights,
            AZStd::span<bool> terrainExists) const;
        void GetNormalsSynchronous(
            const AZStd::span<const AZ::Vector3>& inPositions,
            Sampler sampler, AZStd::span<AZ::Vector3> normals,
            AZStd::span<bool> terrainExists) const;
        void GetOrderedSurfaceWeightsFromList(
            const AZStd::span<const AZ::Vector3>& inPositions, Sampler sampler,
            AZStd::span<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeightsList,
            AZStd::span<bool> terrainExists) const;
        void MakeBulkQueries(
            const AZStd::span<const AZ::Vector3> inPositions,
            AZStd::span<AZ::Vector3> outPositions,
            AZStd::span<bool> outTerrainExists,
            AZStd::span<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWieghts,
            BulkQueriesCallback queryCallback) const;
        void GenerateQueryPositions(const AZStd::span<const AZ::Vector3>& inPositions, 
            AZStd::vector<AZ::Vector3>& outPositions,
            Sampler sampler) const;
        AZStd::vector<AZ::Vector3> GenerateInputPositionsFromRegion(
            const AZ::Aabb& inRegion,
            const AZ::Vector2& stepSize,
            Sampler sampler) const;
        AZStd::vector<AZ::Vector3> GenerateInputPositionsFromListOfVector2(
            const AZStd::span<const AZ::Vector2> inPositionsVec2) const;

        // AZ::TickBus::Handler overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        struct TerrainSystemSettings
        {
            AZ::Aabb m_worldBounds;
            float m_heightQueryResolution{ 1.0f };
            float m_surfaceDataQueryResolution{ 1.0f };
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
        const AZStd::span<const VectorType>& inPositions,
        AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
        Sampler sampler,
        AZStd::shared_ptr<ProcessAsyncParams> params) const
    {
        AZ_PROFILE_FUNCTION(Terrain);

        const int32_t numPositionsToProcess = static_cast<int32_t>(inPositions.size());

        if (numPositionsToProcess == 0)
        {
            AZ_Warning("TerrainSystem", false, "No positions to process.");
            return nullptr;
        }

        // Determine the maximum number of jobs, and the minimum number of positions that should be processed per job.
        const int32_t numJobsMax = CalculateMaxJobs(params);
        const int32_t minPositionsPerJob =
            params && (params->m_minPositionsPerJob > 0) ? params->m_minPositionsPerJob : ProcessAsyncParams::MinPositionsPerJobDefault;

        // Based on the above, we'll create the maximum number of jobs possible that meet both criteria:
        // - processes at least minPositionsPerJob for each job
        // - creates no more than numJobsMax
        const int32_t numJobs = AZStd::clamp(numPositionsToProcess / minPositionsPerJob, 1, numJobsMax);

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
            const AZStd::span<const VectorType>& positionsToProcess = inPositions.subspan(subSpanOffset, subSpanCount);
            auto jobFunction = [this, synchronousFunction, positionsToProcess, perPositionCallback, sampler, jobContext, params]()
            {
                // Process the sub span of positions, unless the associated job context has been cancelled.
                if (!jobContext->IsCancelled())
                {
                    synchronousFunction(positionsToProcess, perPositionCallback, sampler);
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
        Sampler sampler,
        AZStd::shared_ptr<ProcessAsyncParams> params) const
    {
        AZ_PROFILE_FUNCTION(Terrain);

        const auto [numSamplesX, numSamplesY] = GetNumSamplesFromRegion(inRegion, stepSize, sampler);
        const int64_t numPositionsToProcess = numSamplesX * numSamplesY;

        if (numPositionsToProcess == 0)
        {
            AZ_Warning("TerrainSystem", false, "No positions to process.");
            return nullptr;
        }

        // Determine the maximum number of jobs, and the minimum number of positions that should be processed per job.
        const int32_t numJobsMax = CalculateMaxJobs(params);
        const int32_t minPositionsPerJob =
            params && (params->m_minPositionsPerJob > 0) ? params->m_minPositionsPerJob : ProcessAsyncParams::MinPositionsPerJobDefault;

        // Calculate the best subdivision of the region along both the X and Y axes to use as close to the maximum number of jobs
        // as possible while also keeping all the regions effectively the same size.
        int32_t xJobs, yJobs;
        SubdivideRegionForJobs(
            aznumeric_cast<int32_t>(numSamplesX), aznumeric_cast<int32_t>(numSamplesY), numJobsMax, minPositionsPerJob, xJobs, yJobs);

        // The number of jobs returned might be less than the total requested maximum number of jobs, so recalculate it here
        AZ_Assert((xJobs * yJobs) <= numJobsMax, "The region was subdivided into too many jobs: %d x %d vs %d max",
            xJobs, yJobs, numJobsMax);
        const int32_t numJobs = xJobs * yJobs;

        // Get the number of samples in each direction that we'll use for each query. We calculate this as a fractional value
        // so that we can keep each query pretty evenly balanced, with just +/- 1 count variation on each axis.
        const float xSamplesPerQuery = aznumeric_cast<float>(numSamplesX) / xJobs;
        const float ySamplesPerQuery = aznumeric_cast<float>(numSamplesY) / yJobs;

        // Make sure our subdivisions are producing at least minPositionsPerJob unless the *total* requested point count is
        // less than minPositionsPerJob.
        AZ_Assert(
            ((numSamplesX * numSamplesY) < minPositionsPerJob) ||
                (aznumeric_cast<int32_t>(xSamplesPerQuery) * aznumeric_cast<int32_t>(ySamplesPerQuery)) >= minPositionsPerJob,
            "Too few positions per job: %d vs %d", aznumeric_cast<int32_t>(xSamplesPerQuery) * aznumeric_cast<int32_t>(ySamplesPerQuery),
            minPositionsPerJob);

        // Create a terrain job context and split the work across multiple jobs.
        AZStd::shared_ptr<TerrainJobContext> jobContext = AZStd::make_shared<TerrainJobContext>(*m_terrainJobManager, numJobs);
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_activeTerrainJobContextMutex);
            m_activeTerrainJobContexts.push_back(jobContext);
        }

        [[maybe_unused]] int32_t jobsStarted = 0;

        for (int32_t yJob = 0; yJob < yJobs; yJob++)
        {
            // Use the fractional samples per query to calculate the start and end of the region, but then convert it
            // back to integers so that our regions are always in exact multiples of the number of samples to process.
            // This is important because we want the XY values for each point that we're processing to exactly align with
            // 'start + N * (step size)', or else we'll start to process point locations that weren't actually what was requested.
            int32_t y0 = aznumeric_cast<int32_t>(yJob * ySamplesPerQuery);
            int32_t y1 = aznumeric_cast<int32_t>((yJob + 1) * ySamplesPerQuery);
            const float inRegionMinY = inRegion.GetMin().GetY() + (y0 * stepSize.GetY());

            // For the last iteration, just set the end to the max to ensure that floating-point drift doesn't cause us to
            // misalign and miss a point.
            const float inRegionMaxY =
                (yJob == (yJobs - 1)) ? inRegion.GetMax().GetY() : inRegion.GetMin().GetY() + (y1 * stepSize.GetY());

            for (int32_t xJob = 0; xJob < xJobs; xJob++)
            {
                // Same as above, calculate the start and end of the region, then convert back to integers and create the
                // region based on 'start + n * (step size)'.
                int32_t x0 = aznumeric_cast<int32_t>(xJob * xSamplesPerQuery);
                int32_t x1 = aznumeric_cast<int32_t>((xJob + 1) * xSamplesPerQuery);
                const float inRegionMinX = inRegion.GetMin().GetX() + (x0 * stepSize.GetX());
                const float inRegionMaxX =
                    (xJob == (xJobs - 1)) ? inRegion.GetMax().GetX() : inRegion.GetMin().GetX() + (x1 * stepSize.GetX());

                // Define the job function using the sub region of positions to process.
                AZ::Aabb subRegion = AZ::Aabb::CreateFromMinMax(
                    AZ::Vector3(inRegionMinX, inRegionMinY, inRegion.GetMin().GetZ()),
                    AZ::Vector3(inRegionMaxX, inRegionMaxY, inRegion.GetMax().GetZ()));

                auto jobFunction = [this, synchronousFunction, subRegion, stepSize, perPositionCallback, sampler, jobContext, params]()
                {
                    // Process the sub region of positions, unless the associated job context has been cancelled.
                    if (!jobContext->IsCancelled())
                    {
                        synchronousFunction(subRegion, stepSize, perPositionCallback, sampler);
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
                            m_activeTerrainJobContexts.erase(
                                AZStd::find(m_activeTerrainJobContexts.begin(), m_activeTerrainJobContexts.end(), jobContext));
                            m_activeTerrainJobContextMutexConditionVariable.notify_one();
                        }
                    }
                };

                // Create the job and start it immediately.
                AZ::Job* processJob = AZ::CreateJobFunction(jobFunction, true, jobContext.get());
                processJob->Start();
                jobsStarted++;
            }
        }

        // Validate this just to ensure that the fractional math for handling points didn't cause any rounding errors anywhere.
        AZ_Assert(jobsStarted == numJobs, "Wrong number of jobs created: %d vs %d", jobsStarted, numJobs);

        return jobContext;
    }
} // namespace Terrain
