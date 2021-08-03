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
#include <AzCore/std/string/string_view.h>
#include <AzCore/Math/Aabb.h>

class CShader;
namespace SurfaceData
{
    struct SurfacePoint;
    using SurfaceTagWeightMap = AZStd::unordered_map<AZ::Crc32, float>;
}

namespace Terrain
{
    class TerrainDataRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        virtual float GetHeightmapCellSize() = 0;

        virtual float GetHeightSynchronous(float x, float y) = 0;
        virtual AZ::Vector3 GetNormalSynchronous(float x, float y) = 0;

        virtual CShader* GetTerrainHeightGeneratorShader() const = 0;
        virtual CShader* GetTerrainMaterialCompositingShader() const = 0;

        enum class Sampler
        {
            BILINEAR,   // Get the value at the requested location, using terrain sample grid to bilinear filter between sample grid points
            CLAMP,      // Clamp the input point to the terrain sample grid, then get the exact value
            EXACT,       // Directly get the value at the location, regardless of terrain sample grid density

            DEFAULT = BILINEAR
        };

        virtual void GetHeight(const AZ::Vector3& inPosition, Sampler sampleFilter, AZ::Vector3& outPosition) = 0;
        virtual void GetNormal(const AZ::Vector3& inPosition, Sampler sampleFilter, AZ::Vector3& outNormal) = 0;
        virtual void GetSurfaceWeights(const AZ::Vector3& inPosition, Sampler sampleFilter, SurfaceData::SurfaceTagWeightMap& outSurfaceWeights) = 0;
        virtual void GetSurfacePoint(const AZ::Vector3& inPosition, Sampler sampleFilter, SurfaceData::SurfacePoint& outSurfacePoint) = 0;


        typedef AZStd::function<void()> TerrainDataReadyCallback;
        typedef AZStd::function<void(const SurfaceData::SurfacePoint& surfacePoint, uint32_t xIndex, uint32_t yIndex)> SurfacePointRegionFillCallback;

        virtual void ProcessHeightsFromRegion(const AZ::Aabb& inRegion, const AZ::Vector2 stepSize, Sampler sampleFilter, SurfacePointRegionFillCallback perPositionCallback, TerrainDataReadyCallback onComplete = nullptr) = 0;
        virtual void ProcessSurfacePointsFromRegion(const AZ::Aabb& inRegion, const AZ::Vector2 stepSize, Sampler sampleFilter, SurfacePointRegionFillCallback perPositionCallback, TerrainDataReadyCallback onComplete = nullptr) = 0;

    };
    using TerrainDataRequestBus = AZ::EBus<TerrainDataRequests>;
}

class ShaderRequests
    : public AZ::EBusTraits
{
public:
    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    using MutexType = AZStd::recursive_mutex;
    //////////////////////////////////////////////////////////////////////////

    virtual void LoadShader(const AZStd::string_view name, CShader* shader) = 0;
    virtual void UnloadShader(CShader* shader) const = 0;
};
using ShaderRequestBus = AZ::EBus<ShaderRequests>;
