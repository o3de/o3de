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

namespace AZ
{
    namespace Render
    {
        //! Custom data for the RayTracingPass, specified in the PassRequest.
        struct RayTracingPassData
            : public RPI::RenderPassData
        {
            AZ_RTTI(RayTracingPassData, "{26C2E2FD-D30A-4142-82A3-0167BC94B3EE}", RPI::RenderPassData);
            AZ_CLASS_ALLOCATOR(RayTracingPassData, SystemAllocator, 0);

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
                        ->Field("MissShaderAsset", &RayTracingPassData::m_missShaderAssetReference)
                        ->Field("MissShaderName", &RayTracingPassData::m_missShaderName)
                        ->Field("MaxPayloadSize", &RayTracingPassData::m_maxPayloadSize)
                        ->Field("MaxAttributeSize", &RayTracingPassData::m_maxAttributeSize)
                        ->Field("MaxRecursionDepth", &RayTracingPassData::m_maxRecursionDepth)
                        ->Field("Thread Count X", &RayTracingPassData::m_threadCountX)
                        ->Field("Thread Count Y", &RayTracingPassData::m_threadCountY)
                        ->Field("Thread Count Z", &RayTracingPassData::m_threadCountZ)
                        ->Field("Make Fullscreen Pass", &RayTracingPassData::m_makeFullscreenPass)
                        ;
                }
            }

            RPI::AssetReference m_rayGenerationShaderAssetReference;
            AZStd::string m_rayGenerationShaderName;
            RPI::AssetReference m_closestHitShaderAssetReference;
            AZStd::string m_closestHitShaderName;
            RPI::AssetReference m_missShaderAssetReference;
            AZStd::string m_missShaderName;

            uint32_t m_maxPayloadSize = 64;
            uint32_t m_maxAttributeSize = 32;
            uint32_t m_maxRecursionDepth = 1;

            uint32_t m_threadCountX = 1;
            uint32_t m_threadCountY = 1;
            uint32_t m_threadCountZ = 1;

            bool m_makeFullscreenPass = false;
        };
    } // namespace RPI
} // namespace AZ

