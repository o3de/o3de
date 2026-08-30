/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Edit/Utils.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/Utils/Utils.h>
#include <Document/InMemoryShaderCompiler.h>

// From Atom_Asset_Shader.Static. These are the same types the Shader Asset Builder uses, included exactly as the builder includes
// them: the point of the spike is that a tool can call them, so wrapping them would defeat it.
#include <Editor/AzslCompiler.h>
#include <Editor/CommonFiles/Preprocessor.h>
#include <Editor/ShaderBuilderUtility.h>

namespace MaterialCanvas
{
    namespace
    {
        // Copied from a "ShaderPlatformInterface: Executing" line in a Material Canvas job log, so that this measures the work the
        // Asset Processor measures. --full is what makes one invocation produce the HLSL and every reflection document together;
        // asking for them one switch at a time would be several azslc runs and would report a cost the real pipeline never pays.
        const AZStd::vector<AZStd::string> AzslcArguments = {
            "--full", "--Zpr", "--W1", "--strip-unused-srgs", "--root-const=128", "--sc-options", "--namespace=dx"
        };

        // Reconstructed from the "Preprocessor: builder ..." line of a Material Canvas job log. -C keeps comments and -+ enables C++
        // mode; the six defines are the preview pipeline's fidelity reductions, which have to be here or MCPP expands code the real
        // preview shader never sees and the measurement is of a different shader.
        const AZStd::vector<AZStd::string> PreprocessorArguments = {
            "-C", "-+",
            "-DENABLE_AREA_LIGHTS=0", "-DENABLE_DECALS=0", "-DENABLE_SHADOWS=0",
            "-DENABLE_SHADER_DEBUGGING=0", "-DENABLE_LIGHT_CULLING=0", "-DENABLE_ACESCC_COLOR_SPACE=0"
        };

        // The engine and project ShaderLib roots the builder puts on the include path, plus the folder holding the input so that a
        // generated shader can find the azsli files generated beside it.
        AZStd::vector<AZStd::string> BuildIncludePaths(const AZStd::string& inputPath)
        {
            const AZStd::string enginePath = AZ::Utils::GetEnginePath().c_str();
            const AZStd::string projectPath = AZ::Utils::GetProjectPath().c_str();

            AZStd::string inputFolder = inputPath;
            AZ::StringFunc::Path::StripFullName(inputFolder);

            return {
                inputFolder,
                projectPath,
                projectPath + "/ShaderLib",
                enginePath + "/Gems/Atom/RHI/Assets/ShaderLib",
                enginePath + "/Gems/Atom/Feature/Common/Assets/ShaderLib",
                enginePath + "/Gems/Atom/RPI/Assets/ShaderLib",
                enginePath + "/Gems",
            };
        }

        double MillisecondsSince(const AZStd::chrono::steady_clock::time_point& start)
        {
            return AZStd::chrono::duration<double, AZStd::milli>(AZStd::chrono::steady_clock::now() - start).count();
        }

        size_t CountLines(const AZStd::string& path)
        {
            const auto contents = AZ::Utils::ReadFile(path);
            if (!contents.IsSuccess())
            {
                return 0;
            }
            // Counted by hand rather than with a standard algorithm: AzCore's AZStd/algorithm.h does not alias std::count, and
            // reaching for <algorithm> here to save three lines would pull the std namespace into a file that otherwise lives
            // entirely in AZStd.
            size_t lineCount = 0;
            for (const char character : contents.GetValue())
            {
                lineCount += (character == '\n') ? 1 : 0;
            }
            return lineCount;
        }
    } // namespace

    InMemoryShaderSpikeResult RunInMemoryShaderSpike(const AZStd::string& preprocessedAzslPath)
    {
        using namespace AZ::ShaderBuilder;

        InMemoryShaderSpikeResult result;
        const auto totalStart = AZStd::chrono::steady_clock::now();

        auto fail = [&result, &totalStart](const AZStd::string& message)
        {
            result.m_succeeded = false;
            result.m_failure = message;
            result.m_totalMs = MillisecondsSince(totalStart);
            AZ_Warning("MaterialCanvas", false, "In-memory shader spike failed: %s", message.c_str());
            return result;
        };

        auto fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO || !fileIO->Exists(preprocessedAzslPath.c_str()))
        {
            return fail(AZStd::string::format("input does not exist: '%s'", preprocessedAzslPath.c_str()));
        }

        // A temp folder of our own, outside every asset scan folder, so nothing written here is ever seen by the Asset Processor.
        // That is the whole point: this path has to be invisible to the pipeline it is trying to avoid. Note that handing this to
        // the AzslCompiler constructor is not enough on its own -- see the output path built for EmitFullData below.
        const AZ::IO::Path tempFolder = AZ::IO::Path(AZ::Utils::GetProjectPath()) / "user" / "MaterialCanvasInMemorySpike";
        if (!fileIO->Exists(tempFolder.c_str()) && !fileIO->CreatePath(tempFolder.c_str()))
        {
            return fail(AZStd::string::format("could not create temp folder: '%s'", tempFolder.c_str()));
        }

        // --------------------------------------------------------------------------------------------------------------------
        // MCPP, if the input still needs it. This is a library call rather than a process spawn, so unlike azslc it costs nothing
        // to launch and cannot be starved by whatever thread is waiting on it.
        // --------------------------------------------------------------------------------------------------------------------

        AZStd::string azslcInputPath = preprocessedAzslPath;

        if (!preprocessedAzslPath.ends_with(".azslin"))
        {
            // The common header has to go on first. azslc needs the platform's AzslcHeader.azsli -- root constant layout, sampler
            // and binding declarations, the dx namespace setup -- and it is prepended to the source rather than included by it, so
            // preprocessing the raw .azsl skips it entirely and azslc then rejects perfectly valid preprocessed output. That is what
            // the first attempt at this did.
            //
            // The path is relative to the executable folder, the same convention azslc's own path uses, and is what the DX12
            // ShaderPlatformInterface returns from GetAzslHeader. Hardcoded rather than discovered because reaching a
            // ShaderPlatformInterface needs an AssetBuilderSDK::PlatformInfo, which is builder-side context this has no business
            // constructing for a measurement.
            AZStd::string prependedPath = preprocessedAzslPath;
            AZ::RHI::PrependArguments prependArguments;
            prependArguments.m_sourceFile = preprocessedAzslPath.c_str();
            prependArguments.m_prependFile = "Builders/ShaderHeaders/Platform/Windows/DX12/AzslcHeader.azsli";
            prependArguments.m_addSuffixToFileName = "dx12";
            prependArguments.m_destinationFolder = tempFolder.c_str();

            prependedPath = AZ::RHI::PrependFile(prependArguments);

            // PrependFile reports failure by handing back the source path unchanged.
            if (prependedPath == preprocessedAzslPath)
            {
                return fail("the platform AZSL header could not be prepended; check that Builders/ShaderHeaders exists next to the exe");
            }

            PreprocessorData preprocessorOutput;
            const auto preprocessStart = AZStd::chrono::steady_clock::now();
            const bool preprocessed = PreprocessFile(
                prependedPath,
                preprocessorOutput,
                AppendIncludePathsToArgumentList(PreprocessorArguments, BuildIncludePaths(preprocessedAzslPath)),
                true);
            result.m_preprocessMs = MillisecondsSince(preprocessStart);

            if (!preprocessed)
            {
                return fail(AZStd::string::format(
                    "MCPP rejected the input, most likely a missing include path: %s", preprocessorOutput.diagnostics.c_str()));
            }

            result.m_includedFileCount = preprocessorOutput.includedPaths.size();
            for (const char character : preprocessorOutput.code)
            {
                result.m_preprocessedLineCount += (character == '\n') ? 1 : 0;
            }

            azslcInputPath = ShaderBuilderUtility::DumpPreprocessedCode(
                "MaterialCanvas", preprocessorOutput.code, tempFolder.Native(),
                AZ::IO::Path(preprocessedAzslPath).Stem().Native(), "dx12");
            if (azslcInputPath.empty())
            {
                return fail("the preprocessed code could not be written out for azslc");
            }
        }

        // --------------------------------------------------------------------------------------------------------------------
        // azslc. One invocation, producing the HLSL and the ia/om/srg/options/bindingdep documents together.
        // --------------------------------------------------------------------------------------------------------------------

        const AzslCompiler azslc(azslcInputPath, tempFolder.Native());

        // The output path has to be given explicitly, and it is not the tempFolder handed to the constructor: EmitFullData does not
        // use that member. Left empty it reproduces azslc's own default, which is to write every product beside the input file --
        // and the input is a cache product, so the six files azslc emits would land inside Cache/pc while the Asset Processor is
        // watching and locking that folder. Measured cost of getting this wrong: 5,994 ms against an expected 670 ms, essentially
        // all of it contention rather than compilation.
        //
        // EmitFullData derives each sub-product by replacing this path's extension, so what it wants is one path with any extension
        // on it; the .hlsl product keeps its own.
        AZ::IO::Path azslcOutputPath = tempFolder / AZ::IO::Path(azslcInputPath).Stem();
        azslcOutputPath.ReplaceExtension(".hlsl");

        const auto azslcStart = AZStd::chrono::steady_clock::now();
        const auto emitOutcome = azslc.EmitFullData(AzslcArguments, azslcOutputPath.Native());
        result.m_azslcMs = MillisecondsSince(azslcStart);

        if (!emitOutcome.IsSuccess())
        {
            return fail("azslc rejected the input, or EmitFullData does not work outside a builder context");
        }

        const ShaderBuilderUtility::AzslSubProducts::Paths& products = emitOutcome.GetValue();

        // --------------------------------------------------------------------------------------------------------------------
        // Reflection. Reading back what azslc just wrote and turning it into the engine objects a ShaderAsset is built from. This
        // is the part that would have had to be reimplemented had AzslCompiler been confined to the builder gem module.
        // --------------------------------------------------------------------------------------------------------------------

        const auto reflectionStart = AZStd::chrono::steady_clock::now();

        SrgDataContainer srgData;
        {
            const auto srgDocument = AZ::JsonSerializationUtils::ReadJsonFile(products[ShaderBuilderUtility::AzslSubProducts::srg]);
            if (!srgDocument.IsSuccess())
            {
                return fail("the --srg document azslc produced could not be read back");
            }
            if (!azslc.ParseSrgPopulateSrgData(srgDocument.GetValue(), srgData))
            {
                return fail("ParseSrgPopulateSrgData rejected the --srg document");
            }
        }

        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout = AZ::RPI::ShaderOptionGroupLayout::Create();
        {
            const auto optionsDocument =
                AZ::JsonSerializationUtils::ReadJsonFile(products[ShaderBuilderUtility::AzslSubProducts::options]);
            if (!optionsDocument.IsSuccess())
            {
                return fail("the --options document azslc produced could not be read back");
            }

            bool usesSpecializationConstants = false;
            if (!azslc.ParseOptionsPopulateOptionGroupLayout(
                    optionsDocument.GetValue(), shaderOptionGroupLayout, usesSpecializationConstants))
            {
                return fail("ParseOptionsPopulateOptionGroupLayout rejected the --options document");
            }
        }

        result.m_reflectionMs = MillisecondsSince(reflectionStart);

        // --------------------------------------------------------------------------------------------------------------------
        // What came back. Counts rather than contents, because the question is whether the reflection is real, not what is in it:
        // an empty SRG list would parse perfectly well and mean nothing was understood.
        // --------------------------------------------------------------------------------------------------------------------

        result.m_srgCount = srgData.size();
        result.m_shaderOptionCount = shaderOptionGroupLayout ? shaderOptionGroupLayout->GetShaderOptions().size() : 0;
        result.m_hlslLineCount = CountLines(products[ShaderBuilderUtility::AzslSubProducts::hlsl]);
        result.m_totalMs = MillisecondsSince(totalStart);
        result.m_succeeded = true;

        AZ_TracePrintf("MaterialCanvas", "================ in-memory shader spike ================\n");
        AZ_TracePrintf("MaterialCanvas", "  input          : %s\n", preprocessedAzslPath.c_str());
        AZ_TracePrintf("MaterialCanvas", "  MCPP           : %7.0f ms  (%zu lines from %zu files)\n",
            result.m_preprocessMs, result.m_preprocessedLineCount, result.m_includedFileCount);
        AZ_TracePrintf("MaterialCanvas", "  azslc (--full) : %7.0f ms\n", result.m_azslcMs);
        AZ_TracePrintf("MaterialCanvas", "  reflection     : %7.0f ms\n", result.m_reflectionMs);
        AZ_TracePrintf("MaterialCanvas", "  total          : %7.0f ms\n", result.m_totalMs);
        AZ_TracePrintf("MaterialCanvas", "  SRGs           : %zu\n", result.m_srgCount);
        AZ_TracePrintf("MaterialCanvas", "  shader options : %zu\n", result.m_shaderOptionCount);
        AZ_TracePrintf("MaterialCanvas", "  generated HLSL : %zu lines\n", result.m_hlslLineCount);
        AZ_TracePrintf("MaterialCanvas", "\n");
        AZ_TracePrintf("MaterialCanvas", "  Not included, and known from the job log: DXC and dxsc, 298 ms for both stages.\n");
        AZ_TracePrintf("MaterialCanvas", "  Compare against the 2.2 s the same shader takes through the Asset Processor.\n");
        AZ_TracePrintf("MaterialCanvas", "========================================================\n");

        return result;
    }
} // namespace MaterialCanvas

namespace MaterialCanvas
{
    InMemoryMaterialSpikeResult RunInMemoryMaterialSpike(const AZStd::string& materialTypeSourcePath)
    {
        InMemoryMaterialSpikeResult result;
        const auto totalStart = AZStd::chrono::steady_clock::now();

        auto fail = [&result, &totalStart](const AZStd::string& message)
        {
            result.m_succeeded = false;
            result.m_failure = message;
            result.m_totalMs = MillisecondsSince(totalStart);
            AZ_Warning("MaterialCanvas", false, "In-memory material spike failed: %s", message.c_str());
            return result;
        };

        // --------------------------------------------------------------------------------------------------------------------
        // Find and load the intermediate material type. This is PipelineStage's output: the concrete, Direct format material type
        // with its shader list already resolved. MaterialBuilder locates it exactly this way.
        // --------------------------------------------------------------------------------------------------------------------

        const auto locateStart = AZStd::chrono::steady_clock::now();

        result.m_intermediatePath = AZ::RPI::MaterialUtils::PredictIntermediateMaterialTypeSourcePath(materialTypeSourcePath);
        if (result.m_intermediatePath.empty())
        {
            return fail(AZStd::string::format(
                "no intermediate material type is predicted for '%s'. That path should be the abstract .materialtype the canvas "
                "wrote, not the generated one.", materialTypeSourcePath.c_str()));
        }

        auto sourceDataOutcome = AZ::RPI::MaterialUtils::LoadMaterialTypeSourceData(result.m_intermediatePath);
        if (!sourceDataOutcome.IsSuccess())
        {
            return fail(AZStd::string::format(
                "the intermediate material type could not be loaded: '%s'. It is produced by PipelineStage, so it only exists "
                "once the Asset Processor has run that job at least once.", result.m_intermediatePath.c_str()));
        }

        const AZ::RPI::MaterialTypeSourceData intermediateSourceData = sourceDataOutcome.TakeValue();
        result.m_locateMs = MillisecondsSince(locateStart);

        // CreateMaterialTypeAsset asserts and fails on the abstract format, so check first and say why rather than tripping an
        // assert. Abstract means "no explicit shader collection", which is the state the canvas writes and PipelineStage resolves.
        if (intermediateSourceData.GetFormat() != AZ::RPI::MaterialTypeSourceData::Format::Direct)
        {
            return fail(
                "the intermediate material type is not in the Direct format, so it still needs PipelineStage. Check that the path "
                "resolved to the _generated material type rather than back to the source.");
        }

        // --------------------------------------------------------------------------------------------------------------------
        // The FinalStage call itself. Shader references inside are resolved through the asset system, so the shaders must already
        // be built -- which is the one Asset Processor job this design keeps.
        // --------------------------------------------------------------------------------------------------------------------

        const auto createStart = AZStd::chrono::steady_clock::now();
        auto materialTypeAssetOutcome = intermediateSourceData.CreateMaterialTypeAsset(
            AZ::Uuid::CreateRandom(), result.m_intermediatePath, false);
        result.m_createMaterialTypeMs = MillisecondsSince(createStart);

        if (!materialTypeAssetOutcome.IsSuccess())
        {
            return fail(
                "CreateMaterialTypeAsset failed. The usual cause is a shader it references not being built yet, since it resolves "
                "those through the asset system.");
        }

        const AZ::Data::Asset<AZ::RPI::MaterialTypeAsset> materialTypeAsset = materialTypeAssetOutcome.TakeValue();
        if (!materialTypeAsset)
        {
            return fail("CreateMaterialTypeAsset reported success but produced no asset.");
        }

        result.m_propertyCount = materialTypeAsset->GetMaterialPropertiesLayout()
            ? materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyCount()
            : 0;
        // Counted from the source data rather than the asset. MaterialTypeAsset exposes GetGeneralShaderCollection() and a
        // non-const ForAllShaderItems, neither of which is a straight count, and for a material type built through a material
        // pipeline the shaders live in the per-pipeline collections rather than the general one. The intermediate's own
        // declaration is the more useful check anyway: it is exactly what CreateMaterialTypeAsset just consumed.
        result.m_shaderCount = intermediateSourceData.m_shaderCollection.size();
        for (const auto& [pipelineName, pipelineState] : intermediateSourceData.m_pipelineData)
        {
            AZ_UNUSED(pipelineName);
            result.m_shaderCount += pipelineState.m_shaderCollection.size();
        }
        result.m_totalMs = MillisecondsSince(totalStart);
        result.m_succeeded = true;

        AZ_TracePrintf("MaterialCanvas", "=============== in-memory material spike ===============\n");
        AZ_TracePrintf("MaterialCanvas", "  intermediate       : %s\n", result.m_intermediatePath.c_str());
        AZ_TracePrintf("MaterialCanvas", "  locate + load      : %7.0f ms\n", result.m_locateMs);
        AZ_TracePrintf("MaterialCanvas", "  material type asset: %7.0f ms\n", result.m_createMaterialTypeMs);
        AZ_TracePrintf("MaterialCanvas", "  total              : %7.0f ms\n", result.m_totalMs);
        AZ_TracePrintf("MaterialCanvas", "  properties         : %zu\n", result.m_propertyCount);
        AZ_TracePrintf("MaterialCanvas", "  shader collections : %zu\n", result.m_shaderCount);
        AZ_TracePrintf("MaterialCanvas", "\n");
        AZ_TracePrintf("MaterialCanvas", "  Compare against 300 ms of Asset Processor time for the FinalStage job that does this.\n");
        AZ_TracePrintf("MaterialCanvas", "========================================================\n");

        return result;
    }
} // namespace MaterialCanvas

namespace MaterialCanvas
{
    AZ::Data::Asset<AZ::RPI::MaterialTypeAsset> CreateInMemoryMaterialTypeAsset(const AZStd::string& materialTypeSourcePath)
    {
        const AZStd::string intermediatePath =
            AZ::RPI::MaterialUtils::PredictIntermediateMaterialTypeSourcePath(materialTypeSourcePath);
        if (intermediatePath.empty())
        {
            return {};
        }

        auto sourceDataOutcome = AZ::RPI::MaterialUtils::LoadMaterialTypeSourceData(intermediatePath);
        if (!sourceDataOutcome.IsSuccess())
        {
            // Expected while PipelineStage has not run yet for this edit. The caller falls back to the asset system, which waits.
            return {};
        }

        const AZ::RPI::MaterialTypeSourceData intermediateSourceData = sourceDataOutcome.TakeValue();

        // CreateMaterialTypeAsset asserts rather than returning a failure on the abstract format, so this has to be checked here.
        if (intermediateSourceData.GetFormat() != AZ::RPI::MaterialTypeSourceData::Format::Direct)
        {
            return {};
        }

        // Warnings are not elevated to errors: the intermediate can legitimately reference a shader whose asset is still building,
        // and the caller's fallback handles that better than a hard failure would.
        auto materialTypeAssetOutcome =
            intermediateSourceData.CreateMaterialTypeAsset(AZ::Uuid::CreateRandom(), intermediatePath, false);
        if (!materialTypeAssetOutcome.IsSuccess())
        {
            return {};
        }

        return materialTypeAssetOutcome.TakeValue();
    }
} // namespace MaterialCanvas
