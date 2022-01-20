/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/containers/span.h>
#include <AzFramework/SurfaceData/SurfaceData.h>

namespace AzFramework
{
    namespace Terrain
    {
        typedef AZStd::function<void(size_t xIndex, size_t yIndex, const SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)> SurfacePointRegionFillCallback;
        typedef AZStd::function<void(const SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)> SurfacePointListFillCallback;

        //! Shared interface for terrain system implementations
        class TerrainDataRequests
            : public AZ::EBusTraits
        {
        public:
            static void Reflect(AZ::ReflectContext* context);

            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            using MutexType = AZStd::recursive_mutex;
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
            virtual AZ::Vector2 GetTerrainHeightQueryResolution() const = 0;
            virtual void SetTerrainHeightQueryResolution(AZ::Vector2 queryResolution) = 0;

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
            virtual void ProcessHeightsFromList(const AZStd::span<AZ::Vector3>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void ProcessNormalsFromList(const AZStd::span<AZ::Vector3>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void ProcessSurfaceWeightsFromList(const AZStd::span<AZ::Vector3>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void ProcessSurfacePointsFromList(const AZStd::span<AZ::Vector3>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void ProcessHeightsFromListOfVector2(const AZStd::span<AZ::Vector2>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void ProcessNormalsFromListOfVector2(const AZStd::span<AZ::Vector2>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void ProcessSurfaceWeightsFromListOfVector2(const AZStd::span<AZ::Vector2>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;
            virtual void ProcessSurfacePointsFromListOfVector2(const AZStd::span<AZ::Vector2>& inPositions,
                SurfacePointListFillCallback perPositionCallback,
                Sampler sampleFilter = Sampler::DEFAULT) const = 0;

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
        };
        using TerrainDataNotificationBus = AZ::EBus<TerrainDataNotifications>;
    } // namespace Terrain
} // namespace AzFramework

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AzFramework::Terrain::TerrainDataRequests::Sampler, "{D29BB6D7-3006-4114-858D-355EAA256B86}");
} // namespace AZ
