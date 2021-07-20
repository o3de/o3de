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
    namespace RPI
    {
        //! Custom data for the ComputePass. Should be specified in the PassRequest.
        struct ComputePassData
            : public RenderPassData
        {
            AZ_RTTI(ComputePassData, "{C9ADBF8D-34A3-4406-9067-DD17D0564FD8}", RenderPassData);
            AZ_CLASS_ALLOCATOR(ComputePassData, SystemAllocator, 0);

            ComputePassData() = default;
            virtual ~ComputePassData() = default;

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<ComputePassData, RenderPassData>()
                        ->Version(1)
                        ->Field("ShaderAsset", &ComputePassData::m_shaderReference)
                        ->Field("Target Thread Count X", &ComputePassData::m_totalNumberOfThreadsX)
                        ->Field("Target Thread Count Y", &ComputePassData::m_totalNumberOfThreadsY)
                        ->Field("Target Thread Count Z", &ComputePassData::m_totalNumberOfThreadsZ)
                        ->Field("Make Fullscreen Pass", &ComputePassData::m_makeFullscreenPass)
                        ;
                }
            }

            AssetReference m_shaderReference;

            uint32_t m_totalNumberOfThreadsX = 0;
            uint32_t m_totalNumberOfThreadsY = 0;
            uint32_t m_totalNumberOfThreadsZ = 0;

            bool m_makeFullscreenPass = false;
        };
    } // namespace RPI
} // namespace AZ

