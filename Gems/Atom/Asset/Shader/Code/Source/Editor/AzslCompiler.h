/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzslData.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <Atom/RPI.Reflect/Base.h>
#include <Editor/ShaderBuilderUtility.h>

namespace AZ
{
    namespace RPI
    {
        class ShaderOptionGroupLayout;
    }

    namespace RHI
    {
        class ShaderPlatformInterface;
    }

    namespace ShaderBuilder
    {
        class AzslCompiler
        {
        public:
            //! AzslCompiler constructor
            //! @param inputFilePath      The target input file to compile. Should be a valid AZSL file with no preprocessing directives.
            AzslCompiler(const AZStd::string& inputFilePath, const AZStd::string& tempFolder);

            //! compile with --full and generate all .json files
            //! @param outputFile="" will use the input as base path.
            Outcome<ShaderBuilderUtility::AzslSubProducts::Paths> EmitFullData(const AZStd::vector<AZStd::string>& azslcArguments,
                                                                               const AZStd::string& outputFile = "") const;
            //! compile to HLSL independently
            bool EmitShader(AZ::IO::GenericStream& outputStream,
                            const AZStd::string& extraCompilerParams) const;
            //! compile with --ia independently and populate document @output
            bool EmitInputAssembler(rapidjson::Document& output) const;
            //! compile with --om  independently
            bool EmitOutputMerger(rapidjson::Document& output) const;
            //! compile with --srg independently
            bool EmitSrgData(rapidjson::Document& output, const AZStd::string& extraCompilerParams) const;
            //! compile with --option independently
            bool EmitOptionsList(rapidjson::Document& output) const;
            //! compile with --bindingdep independently
            bool EmitBindingDependencies(rapidjson::Document& output) const;

            //! make sense of a --ia json document and fill up the function data
            bool ParseIaPopulateFunctionData(const rapidjson::Document& input, AzslFunctions& functionData) const;
            //! make sense of a --ia json document and fill up the struct data
            bool ParseIaPopulateStructData(const rapidjson::Document& input, const AZStd::string& vertexEntryName, StructData& outStructData) const;
            //! make sense of a --om json document and fill up the struct data
            bool ParseOmPopulateStructData(const rapidjson::Document& input, const AZStd::string& fragmentShaderName, StructData& outStructData) const;
            //! make sense of a --srg json document and fill up the srg data container
            bool ParseSrgPopulateSrgData(const rapidjson::Document& input, SrgDataContainer& outSrgData) const;
            //! make sense of a --option json document and fill up the shader option group layout
            bool ParseOptionsPopulateOptionGroupLayout(const rapidjson::Document& input, RPI::Ptr<RPI::ShaderOptionGroupLayout>& shaderOptionGroupLayout, bool& outSpecializationConstants) const;
            //! make sense of a --bindingdep json documment and fill up the binding dependencies object
            bool ParseBindingdepPopulateBindingDependencies(const rapidjson::Document& input, BindingDependencies& bindingDependencies) const;
            //! make sense of a --srg json document and fill up the root constant data
            bool ParseSrgPopulateRootConstantData(const rapidjson::Document& input, RootConstantData& rootConstantData) const;

            //! retrieve the main input set during construction
            const AZStd::string& GetInputFilePath() const;

        protected:
            bool Compile(const AZStd::string& CompilerParams, const AZStd::string& outputFilePath) const;
            
            enum class AfterRead
            {
                Delete,
                Keep
            };

            enum class BuildResult
            {
                Success,
                VersionError,
                CompilationFailed,
                JsonReadbackFailed,
            };

            BuildResult CompileToFileAndPrepareJsonDocument(
                rapidjson::Document& outputJson,
                const char* compilerCommandSwitch,
                const char* outputExtension,
                AfterRead deleteOutputFileAfterReading = AfterRead::Keep) const;

            BuildResult PrepareJsonDocument(
                rapidjson::Document& outputJson,
                const char* outputExtension) const;

            AZStd::string m_inputFilePath;
            AZStd::string m_tempFolder;
        };
    }
}
