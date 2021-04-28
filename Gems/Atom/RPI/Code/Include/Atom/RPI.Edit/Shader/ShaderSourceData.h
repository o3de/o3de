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
        struct ShaderSourceData
        {
            AZ_TYPE_INFO(AZ::RPI::ShaderSourceData, "{B7F00402-872B-4F82-A210-E1A79A366686}");
            AZ_CLASS_ALLOCATOR(ShaderSourceData, AZ::SystemAllocator, 0);

            static constexpr char Extension[] = "shader";
            static constexpr char Extension2[] = "shader2";

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
            RHI::ShaderCompilerArguments m_compiler;

            AZStd::string m_drawListName;

            ProgramSettings m_programSettings;

            // Raster Shader
            RHI::DepthStencilState m_depthStencilState;
            RHI::RasterState m_rasterState;
            RHI::TargetBlendState m_blendState;

            //! List of RHI Backends (aka ShaderPlatformInterface) for which this shader should not be compiled.
            AZStd::vector<AZStd::string> m_disabledRhiBackends;

            struct SupervariantInfo
            {
                AZ_TYPE_INFO(AZ::RPI::ShaderSourceData::SupervariantInfo, "{1132CF2A-C8AB-4DD2-AA90-3021D49AB955}");

                //! Unique name of the supervariant.
                //! If left empty, the data refers to the default supervariant.
                AZ::Name m_name;

                //! + MCPP Macro definition arguments + AZSLc arguments.
                //! These arguments are added after shader_global_build_options.json & m_compiler.m_azslcAdditionalFreeArguments.
                //! Arguments that start with "-D" are given to MCPP.
                //!     Example: "-DMACRO1 -DMACRO2=3".
                //! all other arguments are given to AZSLc.
                //! Note the arguments are added in addition to the arguments
                //! in <GAME_PROJECT>/Config/shader_global_build_options.json
                AZStd::string m_plusArguments;

                //! Opposite to @m_plusArguments.
                //! - MCPP Macro definition arguments - AZSLc arguments.
                //! Because there are global compilation arguments, this one is useful to remove some of those arguments
                //! in order to customize the compilation of a particular supervariant.
                AZStd::string m_minusArguments;

                //! Helper function. Parses @m_minusArguments and @m_plusArguments, looks for arguments of type -D<name>[=<value>] and returns
                //! a list of <name>.
                AZStd::vector<AZStd::string> GetCombinedListOfMacroDefinitionNames() const;

                //! Helper function. Parses @m_plusArguments, looks for arguments of type "-D<name>[=<value>]" and returns
                //! a list of "<name>[=<value>]".
                AZStd::vector<AZStd::string> GetMacroDefinitionsToAdd() const;

                //! Helper function. Takes AZSLc arguments from @m_minusArguments and @m_plusArguments, removes them from @initialAzslcCompilerArguments.
                //! Takes AZSLc arguments from @m_plusArguments and appends them to @initialAzslcCompilerArguments.
                //! Returns a new string with customized arguments.
                AZStd::string GetCustomizedArgumentsForAzslc(const AZStd::string& initialAzslcCompilerArguments) const;
            };

            //! Optional list of supervariants.
            AZStd::vector<SupervariantInfo> m_supervariants;
        };
        
    } // namespace RPI
} // namespace AZ
