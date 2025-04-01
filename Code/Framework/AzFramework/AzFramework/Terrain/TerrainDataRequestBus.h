/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/EBus/EBusSharedDispatchTraits.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Render/GeometryIntersectionStructures.h>
#include <AzFramework/SurfaceData/SurfaceData.h>

namespace AzFramework
{
    namespace Terrain
    {
        typedef AZStd::function<void(size_t xIndex, size_t yIndex, const SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)> SurfacePointRegionFillCallback;
        typedef AZStd::function<void(const SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)> SurfacePointListFillCallback;

        struct FloatRange
        {
            AZ_TYPE_INFO(FloatRange, "{7E6319B6-1409-4865-8AD1-6F68272A94E9}");

            static void Reflect(AZ::ReflectContext* context);

            float m_min = 0.0f;
            float m_max = 0.0f;

            bool IsValid() const
            {
                return m_min <= m_max;
            }

            static FloatRange CreateNull()
            {
                return { 0.0f, -1.0f };
            }

            bool operator==(const FloatRange& other) const
            {
                return m_min == other.m_min && m_max == other.m_max;
            }

            bool operator!=(const FloatRange& other) const
            {
                return !(*this == other);
            }
        };

        //! Helper structure that defines a query region to use with the QueryRegion / QueryRegionAsync APIs.
        struct TerrainQueryRegion
        {
            AZ_TYPE_INFO(TerrainQueryRegion, "{DE3F634D-9689-4FBC-9F43-A39CFDF425D0}");

            TerrainQueryRegion() = default;
            ~TerrainQueryRegion() = default;

            TerrainQueryRegion(const AZ::Vector3& startPoint, size_t numPointsX, size_t numPointsY, const AZ::Vector2& stepSize);
            TerrainQueryRegion(const AZ::Vector2& startPoint, size_t numPointsX, size_t numPointsY, const AZ::Vector2& stepSize);

            static TerrainQueryRegion CreateFromAabbAndStepSize(const AZ::Aabb& region, const AZ::Vector2& stepSize);

            AZ::Vector3 m_startPoint{ 0.0f }; //! The starting point for the region query
            AZ::Vector2 m_stepSize{ 0.0f }; //! The step size to use for advancing to the next point to query
            size_t m_numPointsX{ 0 }; //! The total number of points to query in the X direction
            size_t m_numPointsY{ 0 }; //! The total number of points to query in the Y direction
        };

        //! A JobContext used to run jobs spawned by calls to the various Query*Async functions.
        class TerrainJobContext : public AZ::JobContext
        {
        public:
            AZ_CLASS_ALLOCATOR(TerrainJobContext, AZ::ThreadPoolAllocator)
            TerrainJobContext(AZ::JobManager& jobManager, int numJobsToComplete)
                : JobContext(jobManager)
                , m_numJobsToComplete(numJobsToComplete)
            {
                AZ_Assert(numJobsToComplete > 0, "Invalid number of jobs: %d", numJobsToComplete);
            }

            // When a terrain job context is cancelled, all associated
            // jobs are still guaranteed to at least begin processing,
            // and if any QueryAsyncParams::m_completionCallback was
            // set it's guaranteed to be called even in the event of a
            // cancellation. If a job only begins processing after its
            // associated job context has been cancelled, no processing
            // will occur and the callback will be invoked immediately,
            // otherwise the job may either run to completion or cease
            // processing early; the callback is invoked in all cases,
            // provided one was specified with the original request.
            void Cancel()
            {
                m_isCancelled = true;
            }

            // Was this TerrainJobContext cancelled?
            bool IsCancelled() const
            {
                return m_isCancelled;
            }

            // Called by the TerrainSystem when a job associated with
            // this TerrainJobContext completes. Returns true if this
            // was the final job to be completed, or false otherwise.
            bool OnJobCompleted()
            {
                return (--m_numJobsToComplete <= 0);
            }

        private:
            AZStd::atomic_int m_numJobsToComplete = 0;
            AZStd::atomic_bool m_isCancelled = false;
        };

        //! Alias for an optional callback function to invoke when the various Query*Async functions complete.
        //! The TerrainJobContext, returned from the original Query*Async function call, is passed as a param
        //! to the callback function so it can be queried to see if the job was cancelled or completed normally.
        typedef AZStd::function<void(AZStd::shared_ptr<TerrainJobContext>)> QueryAsyncCompleteCallback;

        //! A parameter group struct that can optionally be passed to the various Query*Async API functions.
        struct QueryAsyncParams
        {
            //! This constant can be used with m_desiredNumberOfJobs to request the maximum
            //! number of jobs possible for splitting up async terrain requests based on the
            //! maximum number of cores / job threads on the current PC. It's a symbolic number,
            //! not a literal one, since the maximum number of available jobs will change from
            //! machine to machine.
            static constexpr int32_t UseMaxJobs = -1;

            //! This constant is used with m_desiredNumberOfJobs to set the default number of jobs
            //! for splitting up async terrain requests. By default, we use a single job so that the
            //! request runs asynchronously but only consumes a single thread.
            static constexpr int32_t NumJobsDefault = 1;

            //! The desired maximum number of jobs to split async terrain requests into.
            //! The actual number of jobs used will be clamped based on the number of available job manager threads,
            //! the desired number of jobs, and the minimum number of positions that should be processed per job.
            int32_t m_desiredNumberOfJobs = NumJobsDefault;

            //! This constant is used with m_minPositionsPerJob to set the default minimum number
            //! of positions to process per async terrain request job. The higher the number, the more it will
            //! limit the maximum number of simultaneous jobs per async terrain request.
            static constexpr int32_t MinPositionsPerJobDefault = 8;

            //! The minimum number of positions per async terrain request job.
            int32_t m_minPositionsPerJob = MinPositionsPerJobDefault;

            //! The callback function that will be invoked when a call to a Query*Async function completes.
            //! If the job is cancelled, the completion callback will not be invoked.
            QueryAsyncCompleteCallback m_completionCallback = nullptr;
        };

        //! Shared interface for terrain system implementations
        class TerrainDataRequests
            : public AZ::EBusSharedDispatchTraits<TerrainDataRequests>
        {
        public:
            static void Reflect(AZ::ReflectContext* context);

            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            //////////////////////////////////////////////////////////////////////////

            enum class Sampler
            {
                BILINEAR,   //! Get the value at the requested location, using terrain sample grid to bilinear filter between sample grid points.
                CLAMP,      //! Clamp the input point to the terrain sample grid, then get the height at the given grid location.
                EXACT,      //! Directly get the value at the location, regardless of terrain sample grid density.

                DEFAULT = BILINEAR
            };

            static float GetDefaultTerrainHeight() { return 0.0f; }
            static AZ::Vector3 GetDefaultTerrainNormal() { return AZ::Vector3::CreateAxisZ(); }

            // System-level queries to understand world size and resolution
            virtual float GetTerrainHeightQueryResolution() const = 0;
            virtual void SetTerrainHeightQueryResolution(float queryResolution) = 0;
            virtual float GetTerrainSurfaceDataQueryResolution() const = 0;
            virtual void SetTerrainSurfaceDataQueryResolution(float queryResolution) = 0;

            // Returns a bounding box that contains all current terrain areas. There may still be areas inside the bounds which contain no terrain.
            virtual AZ::Aabb GetTerrainAabb() const = 0;

            virtual FloatRange GetTerrainHeightBounds() const = 0;
            virtual void SetTerrainHeightBounds(const FloatRange& heightRange) = 0;

            // Returns true if any terrain area spawner intersects with the provided bounds
            virtual bool TerrainAreaExistsInBounds(const AZ::Aabb& bounds) const = 0;

            //! Returns terrains height in meters at location x,y.
            //! @terrainExistsPtr: Can be nullptr. If != nullptr then, if there's no terrain at location x,y or location x,y is inside
            //!  a terrain HOLE then *terrainExistsPtr will become false, otherwise *terrainExistsPtr will become true.
            virtual float GetHeight(const AZ::Vector3& position, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const = 0;
            virtual float GetHeightFromVector2(
                const AZ::Vector2& position, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const = 0;
            virtual float GetHeightFromFloats(
                float x, float y, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const = 0;

            //! Returns true if there's a hole at location x,y.
            //! Also returns true if there's no terrain data at location x,y.
            virtual bool GetIsHole(const AZ::Vector3& position, Sampler sampleFilter = Sampler::BILINEAR) const = 0;
            virtual bool GetIsHoleFromVector2(const AZ::Vector2& position, Sampler sampleFilter = Sampler::BILINEAR) const = 0;
            virtual bool GetIsHoleFromFloats(float x, float y, Sampler sampleFilter = Sampler::BILINEAR) const = 0;

            // Given an XY coordinate, return the surface normal.
            //! @terrainExists: Can be nullptr. If != nullptr then, if there's no terrain at location x,y or location x,y is inside a
            //! terrain HOLE then *terrainExistsPtr will be set to false, otherwise *terrainExistsPtr will be set to true.
            virtual AZ::Vector3 GetNormal(
                const AZ::Vector3& position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const = 0;
            virtual AZ::Vector3 GetNormalFromVector2(
                const AZ::Vector2& position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const = 0;
            virtual AZ::Vector3 GetNormalFromFloats(
                float x, float y, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const = 0;

            //! Given an XY coordinate, return the max surface type and weight.
            //! @terrainExists: Can be nullptr. If != nullptr then, if there's no terrain at location x,y or location x,y is inside
            //! a terrain HOLE then *terrainExistsPtr will be set to false, otherwise *terrainExistsPtr will be set to true.
            virtual SurfaceData::SurfaceTagWeight GetMaxSurfaceWeight(
                const AZ::Vector3& position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const = 0;
            virtual SurfaceData::SurfaceTagWeight GetMaxSurfaceWeightFromVector2(
                const AZ::Vector2& inPosition, Sampler sampleFilter = Sampler::DEFAULT, bool* terrainExistsPtr = nullptr) const = 0;
            virtual SurfaceData::SurfaceTagWeight GetMaxSurfaceWeightFromFloats(
                float x, float y, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const = 0;

            //! Given an XY coordinate, return the set of surface types and weights. The Vector3 input position version is defined to
            //! ignore the input Z value.
            virtual void GetSurfaceWeights(
                const AZ::Vector3& inPosition,
                SurfaceData::SurfaceTagWeightList& outSurfaceWeights,
                Sampler sampleFilter = Sampler::DEFAULT,
                bool* terrainExistsPtr = nullptr) const = 0;
            virtual void GetSurfaceWeightsFromVector2(
                const AZ::Vector2& inPosition,
                SurfaceData::SurfaceTagWeightList& outSurfaceWeights,
                Sampler sampleFilter = Sampler::DEFAULT,
                bool* terrainExistsPtr = nullptr) const = 0;
            virtual void GetSurfaceWeightsFromFloats(
                float x,
                float y,
                SurfaceData::SurfaceTagWeightList& outSurfaceWeights,
                Sampler sampleFilter = Sampler::DEFAULT,
                bool* terrainExistsPtr = nullptr) const = 0;

            //! Convenience function for  low level systems that can't do a reverse lookup from Crc to string. Everyone else should use
            //! GetMaxSurfaceWeight or GetMaxSurfaceWeightFromFloats.
            //! Not available in the behavior context.
            //! Returns nullptr if the position is inside a hole or outside of the terrain boundaries.
            virtual const char* GetMaxSurfaceName(
                const AZ::Vector3& position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const = 0;

            //! Given an XY coordinate, return all terrain information at that location. The Vector3 input position version is defined
            //! to ignore the input Z value.
            virtual void GetSurfacePoint(
                const AZ::Vector3& inPosition,
                SurfaceData::SurfacePoint& outSurfacePoint,
                Sampler sampleFilter = Sampler::DEFAULT,
                bool* terrainExistsPtr = nullptr) const = 0;
            virtual void GetSurfacePointFromVector2(
                const AZ::Vector2& inPosition,
                SurfaceData::SurfacePoint& outSurfacePoint,
                Sampler sampleFilter = Sampler::DEFAULT,
                bool* terrainExistsPtr = nullptr) const = 0;
            virtual void GetSurfacePointFromFloats(
                float x,
                float y,
                SurfaceData::SurfacePoint& outSurfacePoint,
                Sampler sampleFilter = Sampler::DEFAULT,
                bool* terrainExistsPtr = nullptr) const = 0;

            //! Flags for selecting the combination of data to query when querying a list or region.
            //! The flags determine which subset of data in the SurfacePoint struct will be valid in the FillCallback.
            enum TerrainDataMask : uint8_t
            {
                Heights     = 0b00000001,   //! Query height data
                Normals     = 0b00000010,   //! Query normal data
                SurfaceData = 0b00000100,   //! Query surface types and weights
                All         = 0b11111111    //! Query for every available data channel (heights, normals, surface data)
            };

            //! Given a list of XY coordinates, call the provided callback function with surface data corresponding to each
            //! XY coordinate in the list.
            virtual void QueryList(const AZStd::span<const AZ::Vector3>& inPositions,
                TerrainDataMask requestedData,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void QueryListOfVector2(const AZStd::span<const AZ::Vector2>& inPositions,
                TerrainDataMask requestedData,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;

            //! Given a terrain query region, call the provided callback function with terrain data corresponding to the
            //! coordinates in the region.
            virtual void QueryRegion(
                const TerrainQueryRegion& queryRegion,
                TerrainDataMask requestedData,
                SurfacePointRegionFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;

            //! Get the terrain ray cast entity context id.
            virtual EntityContextId GetTerrainRaycastEntityContextId() const = 0;

            //! Given a ray, return the closest intersection with terrain.
            virtual RenderGeometry::RayResult GetClosestIntersection(const RenderGeometry::RayRequest& ray) const = 0;

            //! Asynchronous versions of the various 'Query*' API functions declared above.
            //! It's the responsibility of the caller to ensure all callbacks are thread-safe.
            virtual AZStd::shared_ptr<TerrainJobContext> QueryListAsync(
                const AZStd::span<const AZ::Vector3>& inPositions,
                TerrainDataMask requestedData,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT,
                AZStd::shared_ptr<QueryAsyncParams> params = nullptr) const = 0;
            virtual AZStd::shared_ptr<TerrainJobContext> QueryListOfVector2Async(
                const AZStd::span<const AZ::Vector2>& inPositions,
                TerrainDataMask requestedData,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT,
                AZStd::shared_ptr<QueryAsyncParams> params = nullptr) const = 0;
            virtual AZStd::shared_ptr<TerrainJobContext> QueryRegionAsync(
                const TerrainQueryRegion& queryRegion,
                TerrainDataMask requestedData,
                SurfacePointRegionFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT,
                AZStd::shared_ptr<QueryAsyncParams> params = nullptr) const = 0;
        };
        using TerrainDataRequestBus = AZ::EBus<TerrainDataRequests>;


        class TerrainDataNotifications
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            //////////////////////////////////////////////////////////////////////////

            enum class TerrainDataChangedMask : uint8_t
            {
                None        = 0b00000000,
                Settings    = 0b00000001,
                HeightData  = 0b00000010,
                ColorData   = 0b00000100,
                SurfaceData = 0b00001000,
                All         = 0b00001111
            };

            virtual void OnTerrainDataCreateBegin() {}
            virtual void OnTerrainDataCreateEnd() {}

            virtual void OnTerrainDataDestroyBegin() {}
            virtual void OnTerrainDataDestroyEnd() {}

            virtual void OnTerrainDataChanged(
                [[maybe_unused]] const AZ::Aabb& dirtyRegion, [[maybe_unused]] TerrainDataChangedMask dataChangedMask)
            {
            }
            
            //! Connection policy that auto-calls OnTerrainDataCreateBegin & OnTerrainDataCreateEnd on connection.
            template<class Bus>
            struct ConnectionPolicy : public AZ::EBusConnectionPolicy<Bus>
            {
                static void Connect(
                    typename Bus::BusPtr& busPtr,
                    typename Bus::Context& context,
                    typename Bus::HandlerNode& handler,
                    typename Bus::Context::ConnectLockGuard& connectLock,
                    const typename Bus::BusIdType& id = 0)
                {
                    AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                    if (TerrainDataRequestBus::HasHandlers())
                    {
                        handler->OnTerrainDataCreateBegin();
                        handler->OnTerrainDataCreateEnd();
                    }
                }
            };
        };
        using TerrainDataNotificationBus = AZ::EBus<TerrainDataNotifications>;

        AZ_DEFINE_ENUM_BITWISE_OPERATORS(TerrainDataNotifications::TerrainDataChangedMask);

    } // namespace Terrain
} // namespace AzFramework

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AzFramework::Terrain::TerrainDataRequests::Sampler, "{D29BB6D7-3006-4114-858D-355EAA256B86}");
} // namespace AZ
