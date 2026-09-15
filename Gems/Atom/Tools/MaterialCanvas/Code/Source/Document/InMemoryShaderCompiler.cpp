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
#include <AzCore/std/algorithm.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/Utils/Utils.h>
#include <Document/InMemoryShaderCompiler.h>

// From Atom_Asset_Shader.Static, included exactly as the Shader Asset Builder includes them.
#include <Editor/AzslCompiler.h>
#include <Editor/CommonFiles/Preprocessor.h>
#include <Editor/ShaderBuilderUtility.h>

#include <Atom/RPI.Edit/Shader/ShaderVariantAssetCreator.h>
#include <Atom/RPI.Reflect/Shader/ShaderAssetCreator.h>

#include <MaterialCanvas_Traits_Platform.h>

#if AZ_TRAIT_MATERIALCANVAS_IN_MEMORY_SHADER_COMPILATION_SUPPORTED
// DX12 ShaderPlatformInterface drives the builder's DXC invocation without a builder context; Windows only (see the traits).
#include <RHI.Builders/ShaderPlatformInterface.h>
#endif

namespace MaterialCanvas
{
    namespace
    {
        // Matches the Asset Processor's azslc command line; --full emits the HLSL and all reflection documents in one run.
        const AZStd::vector<AZStd::string> AzslcArguments = {
            "--full", "--Zpr", "--W1", "--strip-unused-srgs", "--root-const=128", "--sc-options", "--namespace=dx"
        };

        // From the job log's MCPP line: -C keeps comments, -+ is C++ mode, and the defines are the preview's fidelity cuts.
        const AZStd::vector<AZStd::string> PreprocessorArguments = {
            "-C", "-+",
            "-DENABLE_AREA_LIGHTS=0", "-DENABLE_DECALS=0", "-DENABLE_SHADOWS=0",
            "-DENABLE_SHADER_DEBUGGING=0", "-DENABLE_LIGHT_CULLING=0", "-DENABLE_ACESCC_COLOR_SPACE=0"
        };

        // The builder's engine and project ShaderLib include roots, plus the input's folder for azsli generated beside it.
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
            // Counted by hand: AZStd has no count alias, and <algorithm> would pull std into an AZStd-only file.
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

        // Temp folder outside all scan folders so the Asset Processor never sees it; see also the EmitFullData output path below.
        const AZ::IO::Path tempFolder = AZ::IO::Path(AZ::Utils::GetProjectPath()) / "user" / "MaterialCanvasInMemorySpike";
        if (!fileIO->Exists(tempFolder.c_str()) && !fileIO->CreatePath(tempFolder.c_str()))
        {
            return fail(AZStd::string::format("could not create temp folder: '%s'", tempFolder.c_str()));
        }

        // MCPP, if the input still needs it; a library call, so no process launch cost.

        AZStd::string azslcInputPath = preprocessedAzslPath;

        if (!preprocessedAzslPath.ends_with(".azslin"))
        {
            // Prepend the platform's AzslcHeader.azsli first, or azslc rejects the output; hardcoded to what GetAzslHeader returns.
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

        // azslc: one invocation producing the HLSL and the ia/om/srg/options/bindingdep documents.

        const AzslCompiler azslc(azslcInputPath, tempFolder.Native());

        // Explicit output path: by default EmitFullData writes beside the input in Cache/pc, contending with the Asset Processor.
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

        // Reflection: read back azslc's output into the engine objects a ShaderAsset is built from.

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

        // DXC via the DX12 ShaderPlatformInterface; must not run on the main thread, since ExecuteShaderCompiler busy-waits.

        const AZStd::string hlslPath = products[ShaderBuilderUtility::AzslSubProducts::hlsl];

#if AZ_TRAIT_MATERIALCANVAS_IN_MEMORY_SHADER_COMPILATION_SUPPORTED
        {
            const auto dxcStart = AZStd::chrono::steady_clock::now();

            // apiUniqueIndex only indexes per-API data on the built assets, so zero is correct for a single-API run.
            AZ::DX12::ShaderPlatformInterface dx12ShaderPlatformInterface(0);

            // Same as the Asset Processor's preview dxc line: -O1 rather than -O3 (and not the slower -O0).
            AZ::RHI::ShaderBuildArguments shaderBuildArguments;
            shaderBuildArguments.m_dxcArguments = { "-Zpr", "-enable-16bit-types", "-O1" };

            // Hardcoded: a preview forward or transparent shader has exactly these two (the real path reads programSettings).
            const AZStd::vector<AZStd::pair<AZStd::string, AZ::RHI::ShaderHardwareStage>> entryPoints = {
                { "VertexShader", AZ::RHI::ShaderHardwareStage::Vertex },
                { "PixelShader", AZ::RHI::ShaderHardwareStage::Fragment },
            };

            for (const auto& [entryName, stage] : entryPoints)
            {
                AZ::RHI::ShaderPlatformInterface::StageDescriptor descriptor;

                // The DX12 implementation ignores PlatformInfo, so a default one is fine.
                const AssetBuilderSDK::PlatformInfo platformInfo;

                // true for specialization constants, so this measures the dxsc pass the Asset Processor also runs, not just DXC.
                if (!dx12ShaderPlatformInterface.CompilePlatformInternal(
                        platformInfo, hlslPath, entryName, stage, tempFolder.Native(), descriptor, shaderBuildArguments, true))
                {
                    return fail(AZStd::string::format("DXC rejected entry point '%s'", entryName.c_str()));
                }

                result.m_stageByteCodeSizes.emplace_back(entryName, descriptor.m_byteCode.size());

                if (descriptor.m_byProducts.m_dynamicBranchCount !=
                    AZ::RHI::ShaderPlatformInterface::ByProducts::UnknownDynamicBranchCount)
                {
                    result.m_dynamicBranchCount += descriptor.m_byProducts.m_dynamicBranchCount;
                }
            }

            result.m_dxcMs = MillisecondsSince(dxcStart);
        }
#endif

        // What came back: counts rather than contents, since an empty SRG list would parse fine and mean nothing.

        result.m_srgCount = srgData.size();
        result.m_shaderOptionCount = shaderOptionGroupLayout ? shaderOptionGroupLayout->GetShaderOptions().size() : 0;
        result.m_hlslLineCount = CountLines(hlslPath);
        result.m_totalMs = MillisecondsSince(totalStart);
        result.m_succeeded = true;

        AZ_TracePrintf("MaterialCanvas", "================ in-memory shader spike ================\n");
        AZ_TracePrintf("MaterialCanvas", "  input          : %s\n", preprocessedAzslPath.c_str());
        AZ_TracePrintf("MaterialCanvas", "  MCPP           : %7.0f ms  (%zu lines from %zu files)\n",
            result.m_preprocessMs, result.m_preprocessedLineCount, result.m_includedFileCount);
        AZ_TracePrintf("MaterialCanvas", "  azslc (--full) : %7.0f ms\n", result.m_azslcMs);
        AZ_TracePrintf("MaterialCanvas", "  reflection     : %7.0f ms\n", result.m_reflectionMs);
        AZ_TracePrintf("MaterialCanvas", "  DXC            : %7.0f ms\n", result.m_dxcMs);
        AZ_TracePrintf("MaterialCanvas", "  total          : %7.0f ms\n", result.m_totalMs);
        AZ_TracePrintf("MaterialCanvas", "  SRGs           : %zu\n", result.m_srgCount);
        AZ_TracePrintf("MaterialCanvas", "  shader options : %zu\n", result.m_shaderOptionCount);
        AZ_TracePrintf("MaterialCanvas", "  generated HLSL : %zu lines\n", result.m_hlslLineCount);
        for (const auto& [entryName, byteCodeSize] : result.m_stageByteCodeSizes)
        {
            AZ_TracePrintf("MaterialCanvas", "  byte code      : %-14s %zu bytes\n", entryName.c_str(), byteCodeSize);
        }
        AZ_TracePrintf("MaterialCanvas", "  dynamic branches: %zu (both stages)\n", result.m_dynamicBranchCount);
        AZ_TracePrintf("MaterialCanvas", "\n");
        AZ_TracePrintf("MaterialCanvas", "  Still missing before this can reach the viewport: ShaderVariantAssetCreator for the\n");
        AZ_TracePrintf("MaterialCanvas", "  root variant, ShaderAssetCreator::Clone against the existing asset, and\n");
        AZ_TracePrintf("MaterialCanvas", "  MaterialTypeAsset::ForAllShaderItems + TryReplaceShaderAsset to swap it in.\n");
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

        // Find and load the intermediate (Direct format) material type from PipelineStage, as MaterialBuilder does.

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

        // CreateMaterialTypeAsset asserts on the abstract format, so check first and report why.
        if (intermediateSourceData.GetFormat() != AZ::RPI::MaterialTypeSourceData::Format::Direct)
        {
            return fail(
                "the intermediate material type is not in the Direct format, so it still needs PipelineStage. Check that the path "
                "resolved to the _generated material type rather than back to the source.");
        }

        // FinalStage itself: shader references resolve through the asset system, so the shaders must already be built.

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
        // Counted from the source data: the asset has no simple count, and pipeline shaders aren't in the general collection.
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

        // Warnings stay warnings: a referenced shader may still be building, and the caller's fallback handles that.
        auto materialTypeAssetOutcome =
            intermediateSourceData.CreateMaterialTypeAsset(AZ::Uuid::CreateRandom(), intermediatePath, false);
        if (!materialTypeAssetOutcome.IsSuccess())
        {
            return {};
        }

        return materialTypeAssetOutcome.TakeValue();
    }
} // namespace MaterialCanvas

namespace MaterialCanvas
{
    AZ::Data::Asset<AZ::RPI::ShaderAsset> CreateInMemoryShaderAsset(
        [[maybe_unused]] const AZStd::string& azslPath,
        [[maybe_unused]] const AZ::Data::Asset<AZ::RPI::ShaderAsset>& sourceShaderAsset,
        [[maybe_unused]] const AZStd::vector<InMemoryShaderEntryPoint>& entryPoints)
    {
#if !AZ_TRAIT_MATERIALCANVAS_IN_MEMORY_SHADER_COMPILATION_SUPPORTED
        // No ShaderPlatformInterface to drive here; see MaterialCanvas_Traits_Platform.h.
        return {};
#else
        using namespace AZ::ShaderBuilder;

        auto decline = []([[maybe_unused]] const char* reason)
        {
            // Not an error: every failure here means "let the Asset Processor do it", which the caller has available.
            AZ_TracePrintf("MaterialCanvas", "In-memory shader asset declined: %s\n", reason);
            return AZ::Data::Asset<AZ::RPI::ShaderAsset>{};
        };

        if (!sourceShaderAsset.IsReady() || entryPoints.empty())
        {
            return decline("no source shader asset to clone, or no entry points");
        }

        auto fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO || !fileIO->Exists(azslPath.c_str()))
        {
            return decline("the AZSL source does not exist");
        }

        // Outside every asset scan folder, so nothing written here is ever seen by the Asset Processor.
        const AZ::IO::Path tempFolder = AZ::IO::Path(AZ::Utils::GetProjectPath()) / "user" / "MaterialCanvasInMemoryShaders";
        if (!fileIO->Exists(tempFolder.c_str()) && !fileIO->CreatePath(tempFolder.c_str()))
        {
            return decline("the temp folder could not be created");
        }

        // MCPP and azslc, identical to the spike above.

        AZStd::string azslcInputPath = azslPath;

        if (!azslPath.ends_with(".azslin"))
        {
            AZ::RHI::PrependArguments prependArguments;
            prependArguments.m_sourceFile = azslPath.c_str();
            prependArguments.m_prependFile = "Builders/ShaderHeaders/Platform/Windows/DX12/AzslcHeader.azsli";
            prependArguments.m_addSuffixToFileName = "dx12";
            prependArguments.m_destinationFolder = tempFolder.c_str();

            const AZStd::string prependedPath = AZ::RHI::PrependFile(prependArguments);
            if (prependedPath == azslPath)
            {
                return decline("the platform AZSL header could not be prepended");
            }

            PreprocessorData preprocessorOutput;
            if (!PreprocessFile(
                    prependedPath,
                    preprocessorOutput,
                    AppendIncludePathsToArgumentList(PreprocessorArguments, BuildIncludePaths(azslPath)),
                    true))
            {
                return decline("MCPP rejected the input");
            }

            azslcInputPath = ShaderBuilderUtility::DumpPreprocessedCode(
                "MaterialCanvas", preprocessorOutput.code, tempFolder.Native(), AZ::IO::Path(azslPath).Stem().Native(), "dx12");
            if (azslcInputPath.empty())
            {
                return decline("the preprocessed code could not be written out for azslc");
            }
        }

        const AzslCompiler azslc(azslcInputPath, tempFolder.Native());

        AZ::IO::Path azslcOutputPath = tempFolder / AZ::IO::Path(azslcInputPath).Stem();
        azslcOutputPath.ReplaceExtension(".hlsl");

        const auto emitOutcome = azslc.EmitFullData(AzslcArguments, azslcOutputPath.Native());
        if (!emitOutcome.IsSuccess())
        {
            return decline("azslc rejected the input");
        }

        const ShaderBuilderUtility::AzslSubProducts::Paths& products = emitOutcome.GetValue();

        // Interface guard: Clone reuses the source asset's layouts, so give up if the shader option layout changed.

        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout = AZ::RPI::ShaderOptionGroupLayout::Create();
        {
            const auto optionsDocument =
                AZ::JsonSerializationUtils::ReadJsonFile(products[ShaderBuilderUtility::AzslSubProducts::options]);
            if (!optionsDocument.IsSuccess())
            {
                return decline("the --options document could not be read back");
            }

            bool usesSpecializationConstants = false;
            if (!azslc.ParseOptionsPopulateOptionGroupLayout(optionsDocument.GetValue(), shaderOptionGroupLayout, usesSpecializationConstants))
            {
                return decline("ParseOptionsPopulateOptionGroupLayout rejected the --options document");
            }
        }

        const AZ::RPI::ShaderOptionGroupLayout* sourceOptionGroupLayout = sourceShaderAsset->GetShaderOptionGroupLayout();
        if (!sourceOptionGroupLayout || sourceOptionGroupLayout->GetHash() != shaderOptionGroupLayout->GetHash())
        {
            return decline("the shader option layout has changed, so the existing asset is not safe to clone");
        }

        // DXC, one invocation per entry point, then the root variant those stages make up.

        AZ::DX12::ShaderPlatformInterface dx12ShaderPlatformInterface(0);

        AZ::RHI::ShaderBuildArguments shaderBuildArguments;
        shaderBuildArguments.m_dxcArguments = { "-Zpr", "-enable-16bit-types", "-O1" };

        const AZ::Data::Asset<AZ::RPI::ShaderVariantAsset> sourceRootVariant = sourceShaderAsset->GetRootVariantAsset();
        if (!sourceRootVariant.IsReady())
        {
            return decline("the source shader asset has no root variant to match");
        }

        AZ::RPI::ShaderVariantAssetCreator variantCreator;
        variantCreator.Begin(
            sourceRootVariant.GetId(), sourceRootVariant->GetShaderVariantId(), AZ::RPI::RootShaderVariantStableId, false);

        // Entry points compile in parallel, as in ShaderVariantAssetBuilder; the variant creator is only touched afterwards, in order.
        AZStd::vector<AZ::RHI::ShaderPlatformInterface::StageDescriptor> descriptors(entryPoints.size());
        AZStd::vector<bool> compiled(entryPoints.size(), false);

        auto compileEntryPoint = [&](size_t index)
        {
            const AssetBuilderSDK::PlatformInfo platformInfo;
            compiled[index] = dx12ShaderPlatformInterface.CompilePlatformInternal(
                platformInfo,
                products[ShaderBuilderUtility::AzslSubProducts::hlsl],
                entryPoints[index].m_name,
                entryPoints[index].m_stage,
                tempFolder.Native(),
                descriptors[index],
                shaderBuildArguments,
                // Specialization constants: azslc used --sc-options, so skipping dxsc leaves sentinel option values in the DXIL.
                true);
        };

        {
            AZStd::vector<AZStd::thread> compileThreads;
            compileThreads.reserve(entryPoints.size() - 1);
            for (size_t i = 1; i < entryPoints.size(); ++i)
            {
                compileThreads.emplace_back([&compileEntryPoint, i]() { compileEntryPoint(i); });
            }

            compileEntryPoint(0);

            for (AZStd::thread& compileThread : compileThreads)
            {
                compileThread.join();
            }
        }

        for (size_t i = 0; i < entryPoints.size(); ++i)
        {
            if (!compiled[i])
            {
                return decline("DXC rejected an entry point");
            }

            AZ::RHI::Ptr<AZ::RHI::ShaderStageFunction> shaderStageFunction =
                dx12ShaderPlatformInterface.CreateShaderStageFunction(descriptors[i]);
            variantCreator.SetShaderFunction(AZ::RHI::ToRHIShaderStage(entryPoints[i].m_stage), shaderStageFunction);
        }

        AZ::Data::Asset<AZ::RPI::ShaderVariantAsset> rootVariantAsset;
        if (!variantCreator.End(rootVariantAsset) || !rootVariantAsset)
        {
            return decline("the root shader variant could not be created");
        }

        // Clone, keeping the source asset's id so the result can replace it in a material type's shader collection.

        AZ::RPI::ShaderAssetCreator::ShaderSupervariants supervariants;
        {
            AZ::RPI::ShaderAssetCreator::ShaderSupervariant supervariant;
            // The default (unnamed) supervariant; preview shaders declare no others, and Clone needs one entry per source supervariant.
            supervariant.m_name = AZ::Name{};
            supervariant.m_rootVariantAssets.push_back({ dx12ShaderPlatformInterface.GetAPIType(), rootVariantAsset });
            supervariants.push_back(AZStd::move(supervariant));
        }

        AZStd::vector<AZ::RHI::ShaderPlatformInterface*> platformInterfaces = { &dx12ShaderPlatformInterface };

        AZ::RPI::ShaderAssetCreator shaderAssetCreator;
        shaderAssetCreator.Clone(sourceShaderAsset.GetId(), *sourceShaderAsset.Get(), supervariants, platformInterfaces);

        AZ::Data::Asset<AZ::RPI::ShaderAsset> shaderAsset;
        if (!shaderAssetCreator.End(shaderAsset) || !shaderAsset)
        {
            return decline("ShaderAssetCreator::Clone did not produce an asset");
        }

        return shaderAsset;
#endif
    }
} // namespace MaterialCanvas

namespace MaterialCanvas
{
    AZStd::vector<InMemoryShaderRequest> CollectInMemoryShaderRequests(
        const AZ::Data::Asset<AZ::RPI::MaterialTypeAsset>& materialTypeAsset, const AZStd::string& materialTypeSourcePath)
    {
        AZStd::vector<InMemoryShaderRequest> requests;

        if (!materialTypeAsset || materialTypeSourcePath.empty())
        {
            return requests;
        }

        // The intermediate .azsl and .shader sit beside the intermediate material type, so derive their folder from its path.
        const AZStd::string intermediateMaterialTypePath =
            AZ::RPI::MaterialUtils::PredictIntermediateMaterialTypeSourcePath(materialTypeSourcePath);
        if (intermediateMaterialTypePath.empty())
        {
            return requests;
        }

        AZ::IO::Path intermediateFolder{ intermediateMaterialTypePath };
        intermediateFolder = intermediateFolder.ParentPath();

        auto fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            return requests;
        }

        size_t shaderItemCount = 0;
        size_t notReadyCount = 0;
        size_t sourcesMissingCount = 0;

        // Walk both collections: pipeline-built material types keep their shaders in the pipeline payloads, not the general one.
        auto collectFrom = [&](const AZ::RPI::ShaderCollection& shaderCollection)
        {
            for (const auto& shaderItem : shaderCollection)
            {
                ++shaderItemCount;

                const AZ::Data::Asset<AZ::RPI::ShaderAsset>& shaderAsset = shaderItem.GetShaderAsset();
                if (!shaderAsset.IsReady())
                {
                    ++notReadyCount;
                    continue;
                }

                // The shader asset's name is the .shader file stem the pipeline stage used for all three files.
                const AZStd::string shaderStem = shaderAsset->GetName().GetStringView();
                if (shaderStem.empty())
                {
                    continue;
                }

                // A shader can appear in several collections; compile it once so two assets don't race to replace one id.
                const bool alreadyCollected = AZStd::any_of(
                    requests.begin(),
                    requests.end(),
                    [&shaderAsset](const InMemoryShaderRequest& existing)
                    {
                        return existing.m_sourceShaderAsset.GetId() == shaderAsset.GetId();
                    });
                if (alreadyCollected)
                {
                    continue;
                }

                const AZ::IO::Path azslPath = intermediateFolder / (shaderStem + ".azsl");
                const AZ::IO::Path shaderPath = intermediateFolder / (shaderStem + ".shader");

                if (!fileIO->Exists(azslPath.c_str()) || !fileIO->Exists(shaderPath.c_str()))
                {
                    // The pipeline stage has not written this edit's sources yet. Normal, and the caller waits.
                    ++sourcesMissingCount;
                    continue;
                }

                const auto shaderSourceDataOutcome =
                    AZ::ShaderBuilder::ShaderBuilderUtility::LoadShaderDataJson(shaderPath.Native(), false);
                if (!shaderSourceDataOutcome.IsSuccess())
                {
                    continue;
                }

                InMemoryShaderRequest request;
                request.m_sourceShaderAsset = shaderAsset;
                request.m_azslPath = azslPath.Native();
                request.m_shaderPath = shaderPath.Native();

                for (const auto& entryPoint : shaderSourceDataOutcome.GetValue().m_programSettings.m_entryPoints)
                {
                    // Convert RPI::ShaderStageType to RHI::ShaderHardwareStage, as ShaderVariantAssetBuilder does.
                    InMemoryShaderEntryPoint inMemoryEntryPoint;
                    inMemoryEntryPoint.m_name = entryPoint.m_name;
                    inMemoryEntryPoint.m_stage = AZ::ShaderBuilder::ShaderBuilderUtility::ToAssetBuilderShaderType(entryPoint.m_type);
                    request.m_entryPoints.push_back(AZStd::move(inMemoryEntryPoint));
                }

                if (!request.m_entryPoints.empty())
                {
                    requests.push_back(AZStd::move(request));
                }
            }
        };

        collectFrom(materialTypeAsset->GetGeneralShaderCollection());

        for (const auto& [materialPipelineName, materialPipelinePayload] : materialTypeAsset->GetMaterialPipelinePayloads())
        {
            collectFrom(materialPipelinePayload.m_shaderCollection);
        }

        AZ_TracePrintf(
            "MaterialCanvas",
            "In-memory shader collection: %zu shader item(s), %zu not loaded, %zu missing sources, %zu request(s).\n",
            shaderItemCount, notReadyCount, sourcesMissingCount, requests.size());

        return requests;
    }

    AZStd::vector<AZStd::pair<AZ::Data::AssetId, AZ::Data::Asset<AZ::RPI::ShaderAsset>>> CompileInMemoryShaders(
        const AZStd::vector<InMemoryShaderRequest>& requests)
    {
        AZStd::vector<AZStd::pair<AZ::Data::AssetId, AZ::Data::Asset<AZ::RPI::ShaderAsset>>> compiled;

        for (const InMemoryShaderRequest& request : requests)
        {
            const AZ::Data::Asset<AZ::RPI::ShaderAsset> shaderAsset =
                CreateInMemoryShaderAsset(request.m_azslPath, request.m_sourceShaderAsset, request.m_entryPoints);

            if (shaderAsset)
            {
                compiled.emplace_back(request.m_sourceShaderAsset.GetId(), shaderAsset);
            }
        }

        return compiled;
    }
} // namespace MaterialCanvas
