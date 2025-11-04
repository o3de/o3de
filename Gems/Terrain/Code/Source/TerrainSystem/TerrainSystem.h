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
        void RefreshRegion(
            const AZ::Aabb& dirtyRegion, AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask changeMask) override;

        ///////////////////////////////////////////
        // TerrainDataRequestBus::Handler Impl
        float GetTerrainHeightQueryResolution() const override;
        void SetTerrainHeightQueryResolution(float queryResolution) override;
        float GetTerrainSurfaceDataQueryResolution() const override;
        void SetTerrainSurfaceDataQueryResolution(float queryResolution) override;

        virtual AZ::Aabb GetTerrainAabb() const override;

        AzFramework::Terrain::FloatRange GetTerrainHeightBounds() const override;
        void SetTerrainHeightBounds(const AzFramework::Terrain::FloatRange& heightRange) override;

        bool TerrainAreaExistsInBounds(const AZ::Aabb& bounds) const override;

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
        void QueryList(
            const AZStd::span<const AZ::Vector3>& inPositions,
            TerrainDataMask requestedData,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT) const override;
        void QueryListOfVector2(
            const AZStd::span<const AZ::Vector2>& inPositions,
            TerrainDataMask requestedData,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT) const override;

        //! Given a region(aabb) and a step size, call the provided callback function with surface data corresponding to the
        //! coordinates in the region.
        void QueryRegion(
            const AzFramework::Terrain::TerrainQueryRegion& queryRegion,
            TerrainDataMask requestedData,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT) const override;

        AzFramework::EntityContextId GetTerrainRaycastEntityContextId() const override;
        AzFramework::RenderGeometry::RayResult GetClosestIntersection(
            const AzFramework::RenderGeometry::RayRequest& ray) const override;

        AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> QueryListAsync(
            const AZStd::span<const AZ::Vector3>& inPositions,
            TerrainDataMask requestedData,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<AzFramework::Terrain::QueryAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> QueryListOfVector2Async(
            const AZStd::span<const AZ::Vector2>& inPositions,
            TerrainDataMask requestedData,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<AzFramework::Terrain::QueryAsyncParams> params = nullptr) const override;
        AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> QueryRegionAsync(
            const AzFramework::Terrain::TerrainQueryRegion& queryRegion,
            TerrainDataMask requestedData,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<AzFramework::Terrain::QueryAsyncParams> params = nullptr) const override;

    private:
        //! Given a set of async parameters, calculate the max number of jobs that we can use for the async call.
        int32_t CalculateMaxJobs(AZStd::shared_ptr<AzFramework::Terrain::QueryAsyncParams> params) const;

        //! Given the number of samples in a region and the desired number of jobs, choose the best subdivision of the region into jobs.
        static void SubdivideRegionForJobs(
            int32_t numSamplesX, int32_t numSamplesY, int32_t maxNumJobs, int32_t minPointsPerJob,
            int32_t& subdivisionsX, int32_t& subdivisionsY);

        static bool ContainedAabbTouchesEdge(const AZ::Aabb& outerAabb, const AZ::Aabb& innerAabb);

        //! This performs the logic for QueryRegion, but also accepts x and y index offsets so that the subregions for QueryRegionAsync
        //! can pass the correct x and y indices down to the subregion perPositionCallbacks.
        void QueryRegionInternal(
            const AzFramework::Terrain::TerrainQueryRegion& queryRegion,
            size_t xIndexOffset, size_t yIndexOffset,
            TerrainDataMask requestedData,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            Sampler sampler) const;

        template<typename VectorType>
        AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> ProcessFromListAsync(
            const AZStd::span<const VectorType>& inPositions,
            TerrainDataMask requestedData,
            AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
            Sampler sampler = Sampler::DEFAULT,
            AZStd::shared_ptr<AzFramework::Terrain::QueryAsyncParams> params = nullptr) const;

        static void ClampPosition(float x, float y, float queryResolution, AZ::Vector2& outPosition, AZ::Vector2& normalizedDelta);
        static void RoundPosition(float x, float y, float queryResolution, AZ::Vector2& outPosition);
        static void InterpolateHeights(const AZStd::array<float, 4>& heights, const AZStd::array<bool, 4>& exists,
            float lerpX, float lerpY, float& outHeight, bool& outExists);

        AZ::EntityId FindBestAreaEntityAtPosition(const AZ::Vector3& position, AZ::Aabb& bounds) const;
        void GetOrderedSurfaceWeights(
            const float x,
            const float y,
            Sampler sampler,
            AzFramework::SurfaceData::SurfaceTagWeightList& outSurfaceWeights,
            bool* terrainExistsPtr) const;
        float GetHeightSynchronous(float x, float y, Sampler sampler, bool* terrainExistsPtr) const;
        float GetTerrainAreaHeight(float x, float y, bool& terrainExists) const;
        AZ::Vector3 GetNormalSynchronous(const AZ::Vector3& position, Sampler sampler, bool* terrainExistsPtr) const;

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
        void GetNormalsSynchronousExact(
            const AZStd::span<const AZ::Vector3>& inPositions,
            AZStd::span<AZ::Vector3> normals,
            AZStd::span<bool> terrainExists) const;
        void GetNormalsSynchronousClamp(
            const AZStd::span<const AZ::Vector3>& inPositions,
            AZStd::span<AZ::Vector3> normals,
            AZStd::span<bool> terrainExists) const;
        void GetNormalsSynchronousBilinear(
            const AZStd::span<const AZ::Vector3>& inPositions,
            AZStd::span<AZ::Vector3> normals,
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
            AZStd::vector<AZ::Vector3>& outPositions, float queryResolution,
            Sampler sampler) const;
        AZStd::vector<AZ::Vector3> GenerateInputPositionsFromRegion(
            const AzFramework::Terrain::TerrainQueryRegion& queryRegion) const;
        AZStd::vector<AZ::Vector3> GenerateInputPositionsFromListOfVector2(
            const AZStd::span<const AZ::Vector2> inPositionsVec2) const;

        // AZ::TickBus::Handler overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        void RecalculateCachedBounds();
        AZ::Aabb ClampZBoundsToHeightBounds(const AZ::Aabb& aabb) const;

        struct TerrainSystemSettings
        {
            AzFramework::Terrain::FloatRange m_heightRange;
            float m_heightQueryResolution{ 1.0f };
            float m_surfaceDataQueryResolution{ 1.0f };
            bool m_systemActive{ false };
        };

        TerrainSystemSettings m_currentSettings;
        TerrainSystemSettings m_requestedSettings;

        AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask m_terrainDirtyMask =
            AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::Settings;
        AZ::Aabb m_dirtyRegion;
        AZ::Aabb m_cachedAreaBounds;

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
        mutable AZStd::deque<AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext>> m_activeTerrainJobContexts;
    };

    template<typename VectorType>
    inline AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> TerrainSystem::ProcessFromListAsync(
        const AZStd::span<const VectorType>& inPositions,
        TerrainDataMask requestedData,
        AzFramework::Terrain::SurfacePointListFillCallback perPositionCallback,
        Sampler sampler,
        AZStd::shared_ptr<AzFramework::Terrain::QueryAsyncParams> params) const
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
        const int32_t minPositionsPerJob = params && (params->m_minPositionsPerJob > 0)
            ? params->m_minPositionsPerJob
            : AzFramework::Terrain::QueryAsyncParams::MinPositionsPerJobDefault;

        // Based on the above, we'll create the maximum number of jobs possible that meet both criteria:
        // - processes at least minPositionsPerJob for each job
        // - creates no more than numJobsMax
        const int32_t numJobs = AZStd::clamp(numPositionsToProcess / minPositionsPerJob, 1, numJobsMax);

        // Create a terrain job context, track it, and split the work across multiple jobs.
        AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext> jobContext =
            AZStd::make_shared<AzFramework::Terrain::TerrainJobContext>(*m_terrainJobManager, numJobs);
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
            auto jobFunction = [this, positionsToProcess, requestedData, perPositionCallback, sampler, jobContext, params]()
            {
                // Process the sub span of positions, unless the associated job context has been cancelled.
                if (!jobContext->IsCancelled())
                {
                    if constexpr (AZStd::is_same<VectorType, AZ::Vector3>::value)
                    {
                        QueryList(positionsToProcess, requestedData, perPositionCallback, sampler);
                    }
                    else
                    {
                        QueryListOfVector2(positionsToProcess, requestedData, perPositionCallback, sampler);
                    }
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
