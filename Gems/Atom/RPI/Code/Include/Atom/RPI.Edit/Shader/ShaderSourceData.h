/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/PipelineStateDescriptor.h>

#include <Atom/RPI.Edit/Configuration.h>
#include <Atom/RHI.Edit/ShaderPlatformInterface.h>
#include <Atom/RHI.Edit/ShaderBuildArguments.h>
#include <Atom/RPI.Edit/Shader/ShaderOptionValuesSourceData.h>

#include <Atom/RPI.Reflect/Shader/ShaderCommonTypes.h>

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
        struct ATOM_RPI_EDIT_API ShaderSourceData
        {
            AZ_TYPE_INFO(AZ::RPI::ShaderSourceData, "{B7F00402-872B-4F82-A210-E1A79A366686}");
            AZ_CLASS_ALLOCATOR(ShaderSourceData, AZ::SystemAllocator);

            static constexpr char Extension[] = "shader";

            static void Reflect(ReflectContext* context);

            //! Helper function. Returns true if @rhiName is present in m_disabledRhiBackends
            bool IsRhiBackendDisabled(const AZ::Name& rhiName) const;

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

            RHI::ShaderBuildArguments m_removeBuildArguments;  // These arguments are removed first.
            RHI::ShaderBuildArguments m_addBuildArguments;     // Subsequently, these arguments are added.

            // List of macro definitions that will be rolled into m_addBuildArguments.m_preprocessorArguments as command line arguments
            // for the C-Preprocessor. At face value this is redundant, but it is very convenient for the customer
            // as most of the time this all they customize in terms of shader compilation arguments.
            AZStd::vector<AZStd::string> m_definitions;

            // This can override the default shader option values specified in the shader code.
            ShaderOptionValuesSourceData m_shaderOptionValues;

            Name m_drawListName;

            ProgramSettings m_programSettings;

            // Raster Shader
            RHI::DepthStencilState m_depthStencilState;
            RHI::RasterState m_rasterState;

            // Blend States
            RHI::BlendState m_blendState;
            RHI::TargetBlendState m_globalTargetBlendState;
            AZStd::unordered_map<uint32_t, RHI::TargetBlendState> m_targetBlendStates;

            //! List of RHI Backends (aka ShaderPlatformInterface) for which this shader should not be compiled.
            AZStd::vector<AZStd::string> m_disabledRhiBackends;

            struct SupervariantInfo
            {
                AZ_TYPE_INFO(AZ::RPI::ShaderSourceData::SupervariantInfo, "{1132CF2A-C8AB-4DD2-AA90-3021D49AB955}");

                //! Unique name of the supervariant.
                //! If left empty, the data refers to the default supervariant.
                AZ::Name m_name;

                RHI::ShaderBuildArguments m_removeBuildArguments;  // These arguments are removed first.
                RHI::ShaderBuildArguments m_addBuildArguments;     // Subsequently, these arguments are added.

                // List of macro definitions that will be rolled into m_addBuildArguments.m_preprocessorArguments as command line arguments
                // for the C-Preprocessor. At face value this is redundant, but it is very convenient for the customer
                // as most of the time this all they customize in terms of shader compilation arguments.
                AZStd::vector<AZStd::string> m_definitions;
            };

            //! Optional list of supervariants.
            AZStd::vector<SupervariantInfo> m_supervariants;

            //! Typically the AssetProcessor always removes the Temp folder when
            //! an asset compiles successfully.
            //! By setting this flag to true, the Temp folder used to compile this shader won't be deleted
            //! if the shader compiles successfully.
            //! Also, if the ShaderBuildArguments enables shader debug symbols, the Temp
            //! folder won't be removed so it becomes easier to debug shaders with tools like
            //! RenderDoc or Pix. 
            bool m_keepTempFolder = false;
        };
        
    } // namespace RPI
} // namespace AZ
