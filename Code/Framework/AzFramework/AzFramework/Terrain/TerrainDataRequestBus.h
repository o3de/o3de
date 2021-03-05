/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AzFramework
{
    namespace SurfaceData
    {
        struct SurfaceTagWeight
        {
            AZ_TYPE_INFO(SurfaceTagWeight, "{EA14018E-E853-4BF5-8E13-D83BB99A54CC}");

            AZ::Crc32 m_surfaceType;
            float m_weight; //! A Value in the range [0.0f .. 1.0f]

            //! Don't call this directly. TerrainDataRequests::Reflect is doing it already.
            static void Reflect(AZ::ReflectContext* context);
        };
    } //namespace SurfaceData

    namespace Terrain
    {

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
            virtual AZ::Vector2 GetTerrainGridResolution() const = 0;
            virtual AZ::Aabb GetTerrainAabb() const = 0;

            //! Returns terrains height in meters at location x,y.
            //! @terrainExistsPtr: Can be nullptr. If != nullptr then, if there's no terrain at location x,y or location x,y is inside a terrain HOLE then *terrainExistsPtr will become false,
            //!                  otherwise *terrainExistsPtr will become true.
            virtual float GetHeight(AZ::Vector3 position, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const = 0;
            virtual float GetHeightFromFloats(float x, float y, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const = 0;

            //! Given an XY coordinate, return the max surface type and weight.
            //! @terrainExists: Can be nullptr. If != nullptr then, if there's no terrain at location x,y or location x,y is inside a terrain HOLE then *terrainExistsPtr will be set to false,
            //!                  otherwise *terrainExistsPtr will be set to true.
            virtual SurfaceData::SurfaceTagWeight GetMaxSurfaceWeight(AZ::Vector3 position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const = 0;
            virtual SurfaceData::SurfaceTagWeight GetMaxSurfaceWeightFromFloats(float x, float y, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const = 0;

            //! Convenience function for  low level systems that can't do a reverse lookup from Crc to string. Everyone else should use GetMaxSurfaceWeight or GetMaxSurfaceWeightFromFloats.
            //! Not available in the behavior context.
            //! Returns nullptr if the position is inside a hole or outside of the terrain boundaries.
            virtual const char * GetMaxSurfaceName(AZ::Vector3 position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const = 0;

            //! Returns true if there's a hole at location x,y.
            //! Also returns true if there's no terrain data at location x,y.
            virtual bool GetIsHoleFromFloats(float x, float y, Sampler sampleFilter = Sampler::BILINEAR) const = 0;

            // Given an XY coordinate, return the surface normal.
            //! @terrainExists: Can be nullptr. If != nullptr then, if there's no terrain at location x,y or location x,y is inside a terrain HOLE then *terrainExistsPtr will be set to false,
            //!                  otherwise *terrainExistsPtr will be set to true.
            virtual AZ::Vector3 GetNormal(AZ::Vector3 position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const = 0;
            virtual AZ::Vector3 GetNormalFromFloats(float x, float y, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const = 0;
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

            virtual void OnTerrainDataCreateBegin() {};
            virtual void OnTerrainDataCreateEnd() {};

            virtual void OnTerrainDataDestroyBegin() {};
            virtual void OnTerrainDataDestroyEnd() {};
        };
        using TerrainDataNotificationBus = AZ::EBus<TerrainDataNotifications>;

    } //namespace Terrain
} // namespace AzFramework
