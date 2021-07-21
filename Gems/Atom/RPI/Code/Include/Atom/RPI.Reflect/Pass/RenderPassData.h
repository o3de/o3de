/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ShaderDataMappings.h>

#include <Atom/RPI.Reflect/Pass/PassData.h>

namespace AZ
{
    namespace RPI
    {
        //! Custom data for the RenderPass. Should be specified in the PassRequest.
        struct RenderPassData
            : public PassData
        {
            AZ_RTTI(RenderPassData, "{37DE2402-5BAA-48E5-AAC5-3625DFC06BD6}", PassData);
            AZ_CLASS_ALLOCATOR(RenderPassData, SystemAllocator, 0);

            RenderPassData() = default;
            virtual ~RenderPassData() = default;

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<RenderPassData, PassData>()
                        ->Version(1)
                        ->Field("PipelineViewTag", &RenderPassData::m_pipelineViewTag)
                        ->Field("ShaderDataMappings", &RenderPassData::m_mappings);
                }
            }

            //! A grouping of values and value names used to bind data to the per-pass shader resource groups.
            RHI::ShaderDataMappings m_mappings;

            Name m_pipelineViewTag;
        };
    } // namespace RPI
} // namespace AZ

