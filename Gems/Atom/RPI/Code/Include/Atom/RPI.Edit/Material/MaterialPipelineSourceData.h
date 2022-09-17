/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        struct MaterialPipelineSourceData
        {
            AZ_TYPE_INFO(AZ::RPI::MaterialPipelineSourceData, "{AB033EDC-0D89-441C-B9E0-DAFF8058865D}");
            AZ_CLASS_ALLOCATOR(MaterialPipelineSourceData, AZ::SystemAllocator, 0);

            static constexpr char Extension[] = "materialpipeline";

            static void Reflect(ReflectContext* context);

            struct ShaderTemplate
            {
                AZ_TYPE_INFO(AZ::RPI::MaterialPipelineSourceData::ShaderTemplate, "{CC8BAAB1-1C21-4125-A81A-7BB8541494A5}");

                AZStd::string m_shader;

                // This path must be reachable as a #include statement in other azsl files.
                // TODO: See if there's a way we can make this path be relative to the .materialpipeline file instead.
                AZStd::string m_azsli; 
            };

            AZStd::vector<ShaderTemplate> m_shaderTemplates;
        };
        
    } // namespace RPI
} // namespace AZ
