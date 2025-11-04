/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Asset/AssetReference.h>
#include <Atom/RPI.Reflect/Pass/RenderPassData.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        //! Custom data for the RayTracingPass, specified in the PassRequest.
        struct RayTracingPassData
            : public RPI::RenderPassData
        {
            AZ_RTTI(RayTracingPassData, "{26C2E2FD-D30A-4142-82A3-0167BC94B3EE}", RPI::RenderPassData);
            AZ_CLASS_ALLOCATOR(RayTracingPassData, SystemAllocator);

            RayTracingPassData() = default;
            virtual ~RayTracingPassData() = default;

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<RayTracingPassData, RenderPassData>()
                        ->Version(1)
                        ->Field("RayGenerationShaderAsset", &RayTracingPassData::m_rayGenerationShaderAssetReference)
                        ->Field("RayGenerationShaderName", &RayTracingPassData::m_rayGenerationShaderName)
                        ->Field("ClosestHitShaderAsset", &RayTracingPassData::m_closestHitShaderAssetReference)
                        ->Field("ClosestHitShaderName", &RayTracingPassData::m_closestHitShaderName)
                        ->Field("ClosestHitProceduralShaderAsset", &RayTracingPassData::m_closestHitProceduralShaderAssetReference)
                        ->Field("ClosestHitProceduralShaderName", &RayTracingPassData::m_closestHitProceduralShaderName)
                        ->Field("MissShaderAsset", &RayTracingPassData::m_missShaderAssetReference)
                        ->Field("MissShaderName", &RayTracingPassData::m_missShaderName)
                        ->Field("IntersectionShaderAsset", &RayTracingPassData::m_intersectionShaderAssetReference)
                        ->Field("IntersectionShaderName", &RayTracingPassData::m_intersectionShaderName)
                        ->Field("MaxPayloadSize", &RayTracingPassData::m_maxPayloadSize)
                        ->Field("MaxAttributeSize", &RayTracingPassData::m_maxAttributeSize)
                        ->Field("MaxRecursionDepth", &RayTracingPassData::m_maxRecursionDepth)
                        ->Field("ThreadCountX", &RayTracingPassData::m_threadCountX)
                        ->Field("ThreadCountY", &RayTracingPassData::m_threadCountY)
                        ->Field("ThreadCountZ", &RayTracingPassData::m_threadCountZ)
                        ->Field("FullscreenDispatch", &RayTracingPassData::m_fullscreenDispatch)
                        ->Field("FullscreenSizeSourceSlotName", &RayTracingPassData::m_fullscreenSizeSourceSlotName)
                        ->Field("IndirectDispatch", &RayTracingPassData::m_indirectDispatch)
                        ->Field("IndirectDispatchBufferSlotName", &RayTracingPassData::m_indirectDispatchBufferSlotName)
                        ->Field("MaxRayLength", &RayTracingPassData::m_maxRayLength);
                }
            }

            RPI::AssetReference m_rayGenerationShaderAssetReference;
            AZStd::string m_rayGenerationShaderName;
            RPI::AssetReference m_closestHitShaderAssetReference;
            AZStd::string m_closestHitShaderName;
            RPI::AssetReference m_closestHitProceduralShaderAssetReference;
            AZStd::string m_closestHitProceduralShaderName;
            RPI::AssetReference m_missShaderAssetReference;
            AZStd::string m_missShaderName;
            RPI::AssetReference m_intersectionShaderAssetReference;
            AZStd::string m_intersectionShaderName;

            uint32_t m_maxPayloadSize = 64;
            uint32_t m_maxAttributeSize = 32;
            uint32_t m_maxRecursionDepth = 1;
            float m_maxRayLength = 1e27f;

            uint32_t m_threadCountX = 1;
            uint32_t m_threadCountY = 1;
            uint32_t m_threadCountZ = 1;

            bool m_fullscreenDispatch = false;
            AZ::Name m_fullscreenSizeSourceSlotName;

            bool m_indirectDispatch = false;
            AZ::Name m_indirectDispatchBufferSlotName;
        };
    } // namespace RPI
} // namespace AZ

