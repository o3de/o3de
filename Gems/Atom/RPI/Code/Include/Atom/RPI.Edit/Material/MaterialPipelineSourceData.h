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
        //! Describes a material pipeline, which provides shader templates and other mechanisms for automatically
        //! adapting material-specific shader code to work in a specific render pipeline.
        struct MaterialPipelineSourceData
        {
            AZ_TYPE_INFO(AZ::RPI::MaterialPipelineSourceData, "{AB033EDC-0D89-441C-B9E0-DAFF8058865D}");
            AZ_CLASS_ALLOCATOR(MaterialPipelineSourceData, AZ::SystemAllocator, 0);

            static constexpr char Extension[] = "materialpipeline";

            static void Reflect(ReflectContext* context);

            //! Describes a template that will be used to generate the shader asset for one pass in the pipeline.
            struct ShaderTemplate
            {
                AZ_TYPE_INFO(AZ::RPI::MaterialPipelineSourceData::ShaderTemplate, "{CC8BAAB1-1C21-4125-A81A-7BB8541494A5}");

                AZStd::string m_shader; //! Relative path to a template .shader file that will configure the final shader asset.
                AZStd::string m_azsli; //! Relative path to a template .azsli file that will be stitched together with material-specific shader code.

                bool operator==(const ShaderTemplate& rhs) const
                {
                    return m_shader == rhs.m_shader && m_azsli == rhs.m_azsli;
                }
            };

            AZStd::vector<ShaderTemplate> m_shaderTemplates;

            //! Relative path to a lua script to configure shader compilation
            AZStd::string m_pipelineScript;
        };
        
    } // namespace RPI
} // namespace AZ

namespace AZStd
{
    template<typename T>
    struct hash;

    // hashing support for STL containers
    template<>
    struct hash<AZ::RPI::MaterialPipelineSourceData::ShaderTemplate>
    {
        size_t operator()(const AZ::RPI::MaterialPipelineSourceData::ShaderTemplate& value) const
        {
            return hash<AZStd::string>()(value.m_shader) ^ hash<AZStd::string>()(value.m_azsli);
        }
    };
} // namespace AZStd
