/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Asset/AssetReference.h>
#include <Atom/RPI.Reflect/Pass/RenderPassData.h>

namespace AZ
{
    namespace RPI
    {
        //! Custom data for the FullscreenTrianglePass. Should be specified in the PassRequest.
        struct FullscreenTrianglePassData
            : public RenderPassData
        {
            AZ_RTTI(FullscreenTrianglePassData, "{564738A1-9690-4446-8CC1-615EA2567434}", RenderPassData);
            AZ_CLASS_ALLOCATOR(FullscreenTrianglePassData, SystemAllocator, 0);

            FullscreenTrianglePassData() = default;
            virtual ~FullscreenTrianglePassData() = default;

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<FullscreenTrianglePassData, RenderPassData>()
                        ->Version(0)
                        ->Field("ShaderAsset", &FullscreenTrianglePassData::m_shaderAsset)
                        ->Field("StencilRef", &FullscreenTrianglePassData::m_stencilRef);
                }
            }

            //! Reference to the shader asset used by the fullscreen triangle pass
            AssetReference m_shaderAsset;

            //! Stencil reference value to use for the draw call
            uint32_t m_stencilRef = 0;
        };
    } // namespace RPI
} // namespace AZ

