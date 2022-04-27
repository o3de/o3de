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

            virtual AZ::Aabb GetTerrainAabb() const = 0;
            virtual void SetTerrainAabb(const AZ::Aabb& worldBounds) = 0;

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

            //! Given a list of XY coordinates, call the provided callback function with surface data corresponding to each
            //! XY coordinate in the list.
            virtual void ProcessHeightsFromList(const AZStd::span<const AZ::Vector3>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void ProcessNormalsFromList(const AZStd::span<const AZ::Vector3>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void ProcessSurfaceWeightsFromList(const AZStd::span<const AZ::Vector3>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void ProcessSurfacePointsFromList(const AZStd::span<const AZ::Vector3>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void ProcessHeightsFromListOfVector2(const AZStd::span<const AZ::Vector2>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void ProcessNormalsFromListOfVector2(const AZStd::span<const AZ::Vector2>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void ProcessSurfaceWeightsFromListOfVector2(const AZStd::span<const AZ::Vector2>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void ProcessSurfacePointsFromListOfVector2(const AZStd::span<const AZ::Vector2>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;

            //! Returns the number of samples for a given region and step size. The first and second
            //! elements of the pair correspond to the X and Y sample counts respectively.
            virtual AZStd::pair<size_t, size_t> GetNumSamplesFromRegion(const AZ::Aabb& inRegion,
                const AZ::Vector2& stepSize, Sampler samplerType) const = 0;

            //! Given a region(aabb) and a step size, call the provided callback function with surface data corresponding to the
            //! coordinates in the region.
            virtual void ProcessHeightsFromRegion(const AZ::Aabb& inRegion,
                const AZ::Vector2& stepSize,
                SurfacePointRegionFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void ProcessNormalsFromRegion(const AZ::Aabb& inRegion,
                const AZ::Vector2& stepSize,
                SurfacePointRegionFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void ProcessSurfaceWeightsFromRegion(const AZ::Aabb& inRegion,
                const AZ::Vector2& stepSize,
                SurfacePointRegionFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void ProcessSurfacePointsFromRegion(const AZ::Aabb& inRegion,
                const AZ::Vector2& stepSize,
                SurfacePointRegionFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;

            //! Get the terrain raycast entity context id.
            virtual EntityContextId GetTerrainRaycastEntityContextId() const = 0;

            //! Given a ray, return the closest intersection with terrain.
            virtual RenderGeometry::RayResult GetClosestIntersection(const RenderGeometry::RayRequest& ray) const = 0;

            //! A JobContext used to run jobs spawned by calls to the various Process*Async functions.
            class TerrainJobContext : public AZ::JobContext
            {
            public:
                TerrainJobContext(AZ::JobManager& jobManager,
                                  int numJobsToComplete)
                    : JobContext(jobManager)
                    , m_numJobsToComplete(numJobsToComplete)
                {
                }

                // When a terrain job context is cancelled, all associated
                // jobs are still guaranteed to at least begin processing,
                // and if any ProcessAsyncParams::m_completionCallback was
                // set it's guaranteed to be called even in the event of a
                // cancellation. If a job only begins processing after its
                // associated job context has been cancelled, no processing
                // will occur and the callback will be invoked immediately,
                // otherwise the job may either run to completion or cease
                // processing early; the callback is invoked in all cases,
                // provided one was specified with the original request.
                void Cancel() { m_isCancelled = true; }

                // Was this TerrainJobContext cancelled?
                bool IsCancelled() const { return m_isCancelled; }

                // Called by the TerrainSystem when a job associated with
                // this TerrainJobContext completes. Returns true if this
                // was the final job to be completed, or false otherwise.
                bool OnJobCompleted() { return (--m_numJobsToComplete == 0); }

            private:
                AZStd::atomic_int m_numJobsToComplete = 0;
                AZStd::atomic_bool m_isCancelled = false;
            };

            //! Alias for an optional callback function to invoke when the various Process*Async functions complete.
            //! The TerrainJobContext, returned from the original Process*Async function call, is passed as a param
            //! to the callback function so it can be queried to see if the job was cancelled or completed normally.
            typedef AZStd::function<void(AZStd::shared_ptr<TerrainJobContext>)> ProcessAsyncCompleteCallback;

            //! A parameter group struct that can optionally be passed to the various Process*Async API functions.
            struct ProcessAsyncParams
            {
                //! The default minimum  number ofpositions per async terrain request job.
                static constexpr int32_t MinPositionsPerJobDefault = 8;

                //! The default number of jobs which async terrain requests will be split into.
                static constexpr int32_t NumJobsDefault = 1;

                //! The maximum number of jobs which async terrain requests will be split into.
                //! This is not the value itself, rather a constant that can be used to request
                //! the work be split into the maximum number of job manager threads available.
                static constexpr int32_t NumJobsMax = -1;

                //! The desired number of jobs to split async terrain requests into.
                //! The actual value used will be clamped to the number of available job manager threads.
                //!
                //! Note: Currently, splitting the work over multiple threads causes contention when
                //! locking various mutexes, resulting in slower overall wall time for async
                //! requests split over multiple threads vs one where all the work is done on
                //! a single thread. The latter is still preferable over a regular synchronous
                //! call because it is just as quick and prevents the main thread from blocking.
                //! This note should be removed once the mutex contention issues have been addressed.
                int32_t m_desiredNumberOfJobs = NumJobsDefault;

                //! The minimum number of positions per async terrain request job.
                int32_t m_minPositionsPerJob = MinPositionsPerJobDefault;

                //! The callback function that will be invoked when a call to a Process*Async function completes.
                //! If the job is cancelled, the completion callback will not be invoked.
                ProcessAsyncCompleteCallback m_completionCallback = nullptr;
            };

            //! Asynchronous versions of the various 'Process*' API functions declared above.
            //! It's the responsibility of the caller to ensure all callbacks are threadsafe.
            virtual AZStd::shared_ptr<TerrainJobContext> ProcessHeightsFromListAsync(
                const AZStd::span<const AZ::Vector3>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT,
                AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const = 0;
            virtual AZStd::shared_ptr<TerrainJobContext> ProcessNormalsFromListAsync(
                const AZStd::span<const AZ::Vector3>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT,
                AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const = 0;
            virtual AZStd::shared_ptr<TerrainJobContext> ProcessSurfaceWeightsFromListAsync(
                const AZStd::span<const AZ::Vector3>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT,
                AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const = 0;
            virtual AZStd::shared_ptr<TerrainJobContext> ProcessSurfacePointsFromListAsync(
                const AZStd::span<const AZ::Vector3>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT,
                AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const = 0;
            virtual AZStd::shared_ptr<TerrainJobContext> ProcessHeightsFromListOfVector2Async(
                const AZStd::span<const AZ::Vector2>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT,
                AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const = 0;
            virtual AZStd::shared_ptr<TerrainJobContext> ProcessNormalsFromListOfVector2Async(
                const AZStd::span<const AZ::Vector2>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT,
                AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const = 0;
            virtual AZStd::shared_ptr<TerrainJobContext> ProcessSurfaceWeightsFromListOfVector2Async(
                const AZStd::span<const AZ::Vector2>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT,
                AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const = 0;
            virtual AZStd::shared_ptr<TerrainJobContext> ProcessSurfacePointsFromListOfVector2Async(
                const AZStd::span<const AZ::Vector2>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT,
                AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const = 0;
            virtual AZStd::shared_ptr<TerrainJobContext> ProcessHeightsFromRegionAsync(
                const AZ::Aabb& inRegion,
                const AZ::Vector2& stepSize,
                SurfacePointRegionFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT,
                AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const = 0;
            virtual AZStd::shared_ptr<TerrainJobContext> ProcessNormalsFromRegionAsync(
                const AZ::Aabb& inRegion,
                const AZ::Vector2& stepSize,
                SurfacePointRegionFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT,
                AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const = 0;
            virtual AZStd::shared_ptr<TerrainJobContext> ProcessSurfaceWeightsFromRegionAsync(
                const AZ::Aabb& inRegion,
                const AZ::Vector2& stepSize,
                SurfacePointRegionFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT,
                AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const = 0;
            virtual AZStd::shared_ptr<TerrainJobContext> ProcessSurfacePointsFromRegionAsync(
                const AZ::Aabb& inRegion,
                const AZ::Vector2& stepSize,
                SurfacePointRegionFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT,
                AZStd::shared_ptr<ProcessAsyncParams> params = nullptr) const = 0;

        private:
            // Private variations of the GetSurfacePoint API exposed to BehaviorContext that returns a value instead of
            // using an "out" parameter. The "out" parameter is useful for reusing memory allocated in SurfacePoint when
            // using the public API, but can't easily be used from Script Canvas.
            SurfaceData::SurfacePoint BehaviorContextGetSurfacePoint(
                const AZ::Vector3& inPosition,
                Sampler sampleFilter = Sampler::DEFAULT) const
            {
                SurfaceData::SurfacePoint result;
                    GetSurfacePoint(inPosition, result, sampleFilter);
                return result;
            }
            SurfaceData::SurfacePoint BehaviorContextGetSurfacePointFromVector2(
                const AZ::Vector2& inPosition, Sampler sampleFilter = Sampler::DEFAULT) const
            {
                SurfaceData::SurfacePoint result;
                GetSurfacePointFromVector2(inPosition, result, sampleFilter);
                return result;
            }
            // Private variations of the GetHeight.., GetNormal..., GetMaxSurfaceWeight..., GetSurfaceWeights... APIs
            // exposed to BehaviorContext that does not use the terrainExists "out" parameter.
            float BehaviorContextGetHeight(const AZ::Vector3& position, Sampler sampler = Sampler::BILINEAR)
            {
                return GetHeight(position, sampler, nullptr);
            }
            float BehaviorContextGetHeightFromVector2(const AZ::Vector2& position, Sampler sampler = Sampler::BILINEAR)
            {
                return GetHeightFromVector2(position, sampler, nullptr);
            }
            float BehaviorContextGetHeightFromFloats(float x, float y, Sampler sampler = Sampler::BILINEAR)
            {
                return GetHeightFromFloats(x, y, sampler, nullptr);
            }
            AZ::Vector3 BehaviorContextGetNormal(const AZ::Vector3& position, Sampler sampleFilter = Sampler::BILINEAR)
            {
                return GetNormal(position, sampleFilter, nullptr);
            }
            SurfaceData::SurfaceTagWeight BehaviorContextGetMaxSurfaceWeight(
                const AZ::Vector3& position, Sampler sampleFilter = Sampler::BILINEAR)
            {
                return GetMaxSurfaceWeight(position, sampleFilter, nullptr);
            }
            SurfaceData::SurfaceTagWeight BehaviorContextGetMaxSurfaceWeightFromVector2(
                const AZ::Vector2& inPosition, Sampler sampleFilter = Sampler::DEFAULT)
            {
                return GetMaxSurfaceWeightFromVector2(inPosition, sampleFilter, nullptr);
            }
            SurfaceData::SurfaceTagWeightList BehaviorContextGetSurfaceWeights(
                const AZ::Vector3& inPosition,
                Sampler sampleFilter = Sampler::DEFAULT)
            {
                SurfaceData::SurfaceTagWeightList list;
                GetSurfaceWeights(inPosition, list, sampleFilter, nullptr);
                return list;
            }
            SurfaceData::SurfaceTagWeightList BehaviorContextGetSurfaceWeightsFromVector2(
                const AZ::Vector2& inPosition,
                Sampler sampleFilter = Sampler::DEFAULT)
            {
                SurfaceData::SurfaceTagWeightList list;
                GetSurfaceWeightsFromVector2(inPosition, list, sampleFilter, nullptr);
                return list;
            }
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

            enum TerrainDataChangedMask : uint8_t
            {
                None        = 0b00000000,
                Settings    = 0b00000001,
                HeightData  = 0b00000010,
                ColorData   = 0b00000100,
                SurfaceData = 0b00001000
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
    } // namespace Terrain
} // namespace AzFramework

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AzFramework::Terrain::TerrainDataRequests::Sampler, "{D29BB6D7-3006-4114-858D-355EAA256B86}");
} // namespace AZ
