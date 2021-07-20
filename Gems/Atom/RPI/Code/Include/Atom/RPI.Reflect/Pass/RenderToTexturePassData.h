/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Pass/PassData.h>

namespace AZ
{
    namespace RPI
    {
        struct RenderToTexturePassData
            : public PassData
        {
            AZ_RTTI(RenderToTexturePassData, "{A2DEDE8A-C203-4BEA-896F-7F29A58F6978}", PassData);
            AZ_CLASS_ALLOCATOR(RenderToTexturePassData, SystemAllocator, 0);

            RenderToTexturePassData() = default;
            virtual ~RenderToTexturePassData() = default;

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<RenderToTexturePassData, PassData>()
                        ->Version(0)
                        ->Field("OutputWidth", &RenderToTexturePassData::m_width)
                        ->Field("OutputHeight", &RenderToTexturePassData::m_height)
                        ->Field("OutputFormat", &RenderToTexturePassData::m_format);
                }
            }

            uint32_t m_width = 256;
            uint32_t m_height = 256;
            RHI::Format m_format = RHI::Format::R8G8B8A8_UNORM;
        };
    } // namespace RPI
} // namespace AZ

