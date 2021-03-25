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

#include <Atom/RHI/PipelineStateDescriptor.h>

#include <Atom/RHI.Edit/ShaderPlatformInterface.h>
#include <Atom/RHI.Edit/ShaderCompilerArguments.h>

#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace RPI
    {
        //! This is a simple data structure that represents a .shader file.
        //! Provides configuration data about how to compile AZSL code.
        struct ShaderSourceData
        {
            AZ_TYPE_INFO(AZ::RPI::ShaderSourceData, "{B7F00402-872B-4F82-A210-E1A79A366686}");
            AZ_CLASS_ALLOCATOR(ShaderSourceData, AZ::SystemAllocator, 0);

            static const char* Extension;

            static void Reflect(ReflectContext* context);

            //! Helper function. Returns true if @rhiName is present in m_disabledRhiBackends
            bool IsRhiBackendDisabled(const AZ::Name& rhiName);

            struct EntryPoint
            {
                AZ_TYPE_INFO(AZ::RPI::ShaderSourceData::EntryPoint, "{90DB2AEB-9666-42EC-A6D0-A17522A1C4F8}");

                AZStd::string m_name;
                RPI::ShaderStageType m_type;
            };

            struct ProgramSettings
            {
                AZ_TYPE_INFO(AZ::RPI::ShaderSourceData::ProgramSettings, "{660CAC2D-0959-4C34-8A20-465D7AB12E4C}");

                AZStd::vector<EntryPoint> m_entryPoints;
            };

            AZStd::string m_source;
            RHI::ShaderCompilerArguments m_compiler;

            AZStd::string m_drawListName;

            ProgramSettings m_programSettings;

            // Raster Shader
            RHI::DepthStencilState m_depthStencilState;
            RHI::RasterState m_rasterState;
            RHI::TargetBlendState m_blendState;
            
            // Hints for building the shader option group layout
            RPI::ShaderOptionGroupHints m_shaderOptionGroupHints;

            //! List of RHI Backends (aka ShaderPlatformInterface) for which this shader should not be compiled.
            AZStd::vector<AZStd::string> m_disabledRhiBackends;
        };
        
    } // namespace RPI
} // namespace AZ
