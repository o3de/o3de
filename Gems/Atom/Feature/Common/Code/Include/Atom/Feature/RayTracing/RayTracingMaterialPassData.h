/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Pass/RenderPassData.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        //! Custom data for the RayTracingMaterialPass, specified in the PassRequest.
        struct RayTracingMaterialPassData : public RPI::RenderPassData
        {
            AZ_RTTI(RayTracingMaterialPassData, "{C6E5AAE2-F55F-48E0-B907-9D6372E89736}", RPI::RenderPassData);
            AZ_CLASS_ALLOCATOR(RayTracingMaterialPassData, SystemAllocator);

            RayTracingMaterialPassData() = default;
            virtual ~RayTracingMaterialPassData() = default;

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<RayTracingMaterialPassData, RenderPassData>()
                        ->Version(0)
                        ->Field("DrawListTag", &RayTracingMaterialPassData::m_drawListTag)
                        ->Field("MaxPayloadSize", &RayTracingMaterialPassData::m_maxPayloadSize)
                        ->Field("MaxAttributeSize", &RayTracingMaterialPassData::m_maxAttributeSize)
                        ->Field("MaxRecursionDepth", &RayTracingMaterialPassData::m_maxRecursionDepth)
                        ->Field("ThreadCountX", &RayTracingMaterialPassData::m_threadCountX)
                        ->Field("ThreadCountY", &RayTracingMaterialPassData::m_threadCountY)
                        ->Field("ThreadCountZ", &RayTracingMaterialPassData::m_threadCountZ)
                        ->Field("FullscreenDispatch", &RayTracingMaterialPassData::m_fullscreenDispatch)
                        ->Field("FullscreenSizeSourceSlotName", &RayTracingMaterialPassData::m_fullscreenSizeSourceSlotName)
                        ->Field("IndirectDispatch", &RayTracingMaterialPassData::m_indirectDispatch)
                        ->Field("IndirectDispatchBufferSlotName", &RayTracingMaterialPassData::m_indirectDispatchBufferSlotName)
                        ->Field("MaxRayLength", &RayTracingMaterialPassData::m_maxRayLength);
                }
            }

            Name m_drawListTag;

            // TODO: these should come from the Shader
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
    } // namespace Render
} // namespace AZ
