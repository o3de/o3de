/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                        ->Version(2)
                        ->Field("DrawListTag", &RasterPassData::m_drawListTag)
                        ->Field("PassSrgAsset", &RasterPassData::m_passSrgReference)
                        ->Field("Viewport", &RasterPassData::m_overrideViewport)
                        ->Field("Scissor", &RasterPassData::m_overrideScissor)
                        ->Field("DrawListSortType", &RasterPassData::m_drawListSortType)
                        ;
                }
            }

            Name m_drawListTag;

            //! Reference to the per pass SRG asset. This will be used to load the SRG in the raster pass.
            AssetReference m_passSrgReference;

            RHI::Viewport m_overrideViewport = RHI::Viewport::CreateNull();
            RHI::Scissor m_overrideScissor = RHI::Scissor::CreateNull();

            RHI::DrawListSortType m_drawListSortType = RHI::DrawListSortType::KeyThenDepth;
        };
    } // namespace RPI
} // namespace AZ

