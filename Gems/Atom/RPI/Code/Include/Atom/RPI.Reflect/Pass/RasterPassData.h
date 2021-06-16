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

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>

#include <Atom/RPI.Reflect/Asset/AssetReference.h>
#include <Atom/RPI.Reflect/Pass/RenderPassData.h>

namespace AZ
{
    namespace RPI
    {
        //! Custom data for the RasterPass. Should be specified in the PassRequest.
        struct RasterPassData
            : public RenderPassData
        {
            AZ_RTTI(RasterPassData, "{48AAC4A1-EFD5-46E8-9376-E08243F88F54}", RenderPassData);
            AZ_CLASS_ALLOCATOR(RasterPassData, SystemAllocator, 0);

            RasterPassData() = default;
            virtual ~RasterPassData() = default;

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<RasterPassData, RenderPassData>()
                        ->Version(3) // ATOM-15472
                        ->Field("DrawListTag", &RasterPassData::m_drawListTag)
                        ->Field("PassSrgShaderAsset", &RasterPassData::m_passSrgShaderReference)
                        ->Field("Viewport", &RasterPassData::m_overrideViewport)
                        ->Field("Scissor", &RasterPassData::m_overrideScissor)
                        ->Field("DrawListSortType", &RasterPassData::m_drawListSortType)
                        ;
                }
            }

            Name m_drawListTag;

            //! Reference to the shader asset that contains the PassSrg. This will be used to load the SRG in the raster pass.
            AssetReference m_passSrgShaderReference;

            RHI::Viewport m_overrideViewport = RHI::Viewport::CreateNull();
            RHI::Scissor m_overrideScissor = RHI::Scissor::CreateNull();

            RHI::DrawListSortType m_drawListSortType = RHI::DrawListSortType::KeyThenDepth;
        };
    } // namespace RPI
} // namespace AZ

