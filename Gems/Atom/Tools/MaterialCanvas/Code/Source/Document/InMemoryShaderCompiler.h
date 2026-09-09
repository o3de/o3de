/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Edit/ShaderPlatformInterface.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/utils.h>

namespace MaterialCanvas
{
    //! Spike: builds the reflection half of a shader in process, with the Asset Processor out of the picture entirely.
    //!
    //! Material Canvas currently reaches a preview shader by writing files and waiting for the Asset Processor to run four jobs over
    //! them. Measured on this machine that round trip is about 2.2 seconds, of which roughly 970 ms is the compilers doing real work
    //! (azslc 670, DXC and dxsc 300) and roughly 1.2 seconds is the asset pipeline: four out of process builder jobs, the catalog, the
    //! dependency chain, and the files themselves.
    //!
    //! The question this answers is whether Material Canvas can call the same code the builder calls and skip that 1.2 seconds. It is
    //! deliberately not the whole path. It stops before DXC and before the asset creators, because what has to be proven first is
    //! cheap to prove and expensive to assume:
    //!
    //!   1. That a tool can link Atom_Asset_Shader.Static at all. AzslCompiler and ShaderBuilderUtility live in a static library
    //!      rather than only in the builder gem module, so this should work, but nothing else in the tree does it.
    //!   2. That AzslCompiler behaves outside the builder context it was written for.
    //!   3. That its JSON reflection parses into real engine objects -- SRG data, a shader option group layout -- from a tool process.
    //!   4. What the compiler half actually costs when nothing is waiting on the Asset Processor.
    //!
    //! If those hold, the remaining work is DXC (a process spawn with known arguments, measured at 258 ms in the job log),
    //! ShaderAssetCreator and ShaderVariantAssetCreator, MaterialTypeSourceData::CreateMaterialTypeAsset, and the viewport hand off.
    //! The viewport hand off is the only piece with no existing example to copy, because MaterialCanvasViewportContent resolves its
    //! material through an asset id and an in memory asset has no catalog entry.
    //!
    //! Nothing here changes how anything compiles. It reads one file and writes only into a temp folder outside every scan folder.
    struct InMemoryShaderSpikeResult
    {
        bool m_succeeded = false;

        //! What failed, and when. Empty on success.
        AZStd::string m_failure;

        //! Wall clock for each stage, in milliseconds. Stages that ran are reported even when a later one failed.
        double m_preprocessMs = 0.0; //!< MCPP, in process. Zero when the input was already preprocessed.
        double m_azslcMs = 0.0;      //!< One azslc invocation with --full, producing HLSL and every reflection document at once.
        double m_reflectionMs = 0.0; //!< Reading those documents back and turning them into engine objects.
        double m_dxcMs = 0.0;        //!< DXC over every entry point, through the DX12 ShaderPlatformInterface. Windows only.
        double m_totalMs = 0.0;

        //! What came back, as a check that the reflection is real rather than merely parsed.
        size_t m_srgCount = 0;
        size_t m_shaderOptionCount = 0;
        size_t m_hlslLineCount = 0;
        size_t m_preprocessedLineCount = 0;
        size_t m_includedFileCount = 0;

        //! Bytecode produced per entry point, in the order they were compiled. Empty where DXC did not run: off Windows there is no
        //! ShaderPlatformInterface to drive, and the spike reports the stages before it rather than failing.
        AZStd::vector<AZStd::pair<AZStd::string, size_t>> m_stageByteCodeSizes;
        size_t m_dynamicBranchCount = 0;
    };

    //! Runs the spike against either a .azsl or an already preprocessed .azslin, both of which the Asset Processor keeps as cache
    //! products. Given a .azsl it runs MCPP first and reports that separately; given a .azslin it starts at azslc.
    //!
    //! MCPP is worth measuring rather than inferring. The shader job takes 1,197 ms of which 1,097 ms is measured tool time, so its
    //! non-tool overhead is only about 100 ms -- and MCPP is inside that 100 ms along with the Asset Processor's own per job cost.
    //! Which of the two dominates decides whether an in-memory path is worth building, and the job log cannot separate them.
    //!
    //! Logs a per stage breakdown through AZ_TracePrintf under the "MaterialCanvas" window whatever the outcome.
    InMemoryShaderSpikeResult RunInMemoryShaderSpike(const AZStd::string& preprocessedAzslPath);
    //! Spike: builds the MaterialTypeAsset in process from the intermediate material type the Asset Processor already produced,
    //! which is what the FinalStage job does.
    //!
    //! Measured per Material Canvas edit, FinalStage and the MaterialBuilder job cost 300 ms and 268 ms of Asset Processor time
    //! between them, for 22 ms and 28 ms of actual builder work and a 34 ms builder round trip each. Roughly 500 ms is the Asset
    //! Processor working around builders that barely do anything: hashing, dependency fingerprinting, product copies, catalog and
    //! database updates. None of that is computation an in-process path would have to repeat.
    //!
    //! Unlike PipelineStage, this half needs nothing reimplemented. MaterialTypeSourceData::CreateMaterialTypeAsset is public
    //! RPI.Edit API and is the same call FinalStage makes, so there is no second copy of anything to drift out of sync.
    //!
    //! This measures and validates only. It builds the asset and throws it away; wiring it to the viewport is the next step, and
    //! the only part of this with no existing example to copy.
    struct InMemoryMaterialSpikeResult
    {
        bool m_succeeded = false;
        AZStd::string m_failure;

        double m_locateMs = 0.0;             //!< Resolving and loading the intermediate material type.
        double m_createMaterialTypeMs = 0.0; //!< CreateMaterialTypeAsset, i.e. what FinalStage spends its 22 ms on.
        double m_totalMs = 0.0;

        AZStd::string m_intermediatePath;
        size_t m_propertyCount = 0;
        size_t m_shaderCount = 0;
    };

    //! @param materialTypeSourcePath the abstract .materialtype the canvas wrote, e.g. Assets/MaterialCanvasPreview/.../foo.materialtype.
    //! The intermediate is found from it the same way MaterialBuilder finds it, through PredictIntermediateMaterialTypeSourcePath.
    InMemoryMaterialSpikeResult RunInMemoryMaterialSpike(const AZStd::string& materialTypeSourcePath);
    //! Builds a MaterialTypeAsset in process from the intermediate material type the Asset Processor already produced, which is what
    //! the FinalStage job does for 300 ms of Asset Processor time and 16 ms of actual work.
    //!
    //! Returns an invalid asset on any failure, having reported why. Callers are expected to fall back to the asset system path,
    //! because every reason this can fail is a reason the assets are not ready yet rather than a reason they never will be.
    //!
    //! @param materialTypeSourcePath the abstract .materialtype the canvas wrote. The intermediate is located from it the same way
    //! MaterialBuilder locates it, through MaterialUtils::PredictIntermediateMaterialTypeSourcePath.
    AZ::Data::Asset<AZ::RPI::MaterialTypeAsset> CreateInMemoryMaterialTypeAsset(const AZStd::string& materialTypeSourcePath);

    //! One entry point of a shader: the function name azslc and DXC are pointed at, and which hardware stage it is.
    struct InMemoryShaderEntryPoint
    {
        AZStd::string m_name;
        AZ::RHI::ShaderHardwareStage m_stage = AZ::RHI::ShaderHardwareStage::Invalid;
    };

    //! Builds a ShaderAsset in process from AZSL source, by compiling it and then cloning @sourceShaderAsset with the result.
    //!
    //! Cloning rather than building from nothing is what makes this tractable. A ShaderAsset carries far more than byte code -- SRG
    //! layouts, a pipeline layout descriptor, input and output contracts, render states, the shader option group layout -- and
    //! assembling all of that is what ShaderAssetBuilder::ProcessJob does. ShaderAssetCreator::Clone copies every one of those from
    //! an existing asset and replaces only the variants, so the only thing that has to be produced here is the root variant's byte
    //! code. Nothing is reimplemented and nothing can drift.
    //!
    //! The price is the reason this cannot be used unconditionally: everything Clone copies describes the shader's *interface*, and
    //! it comes from @sourceShaderAsset rather than from the source just compiled. That is correct for an edit that changes only
    //! shader code -- the overwhelmingly common case while authoring a graph, and the one worth making fast -- and wrong for an edit
    //! that adds a resource, changes an SRG or changes the shader options. The caller must therefore treat a null return as "use the
    //! Asset Processor", not as an error, and this function returns null whenever it can tell the interface has moved.
    //!
    //! @param azslPath a .azsl or an already preprocessed .azslin. Given a .azsl the platform header is prepended and MCPP runs first.
    //! @param sourceShaderAsset the asset to clone. Its AssetId is kept, so the result can be handed to
    //!        ShaderCollection::Item::TryReplaceShaderAsset, which only accepts an asset whose id matches the one it is replacing.
    //! @param entryPoints the shader's entry points, in any order.
    //!
    //! Runs the compilers as child processes, so it must not be called on the main thread.
    AZ::Data::Asset<AZ::RPI::ShaderAsset> CreateInMemoryShaderAsset(
        const AZStd::string& azslPath,
        const AZ::Data::Asset<AZ::RPI::ShaderAsset>& sourceShaderAsset,
        const AZStd::vector<InMemoryShaderEntryPoint>& entryPoints);

    //! One shader of a material type, paired with everything needed to rebuild it in process.
    struct InMemoryShaderRequest
    {
        AZ::Data::Asset<AZ::RPI::ShaderAsset> m_sourceShaderAsset; //!< What to clone, and whose AssetId the result keeps.
        AZStd::string m_azslPath;                                  //!< The intermediate .azsl the material pipeline stage wrote.
        AZStd::string m_shaderPath;                                //!< The matching .shader, for asking the Asset Processor to rebuild.
        AZStd::vector<InMemoryShaderEntryPoint> m_entryPoints;     //!< Read from the matching intermediate .shader.
    };

    //! Works out which shaders a material type is built from and where their sources are, so they can be recompiled in process.
    //!
    //! The sources are the intermediate .azsl and .shader that MaterialTypeBuilder's pipeline stage writes. They exist well before
    //! the shader assets themselves do -- the pipeline stage runs first and takes about 350 ms, the shader jobs follow -- which is
    //! the whole opportunity: everything needed to build the shader is on disk long before the Asset Processor has finished
    //! building it.
    //!
    //! Returns an empty list when anything is missing, which is the normal state before the pipeline stage has run for this edit.
    AZStd::vector<InMemoryShaderRequest> CollectInMemoryShaderRequests(
        const AZ::Data::Asset<AZ::RPI::MaterialTypeAsset>& materialTypeAsset, const AZStd::string& materialTypeSourcePath);

    //! Recompiles every request and hands back the shaders that succeeded, keyed by the AssetId they replace.
    //!
    //! Runs the compilers as child processes, so it must not be called on the main thread.
    AZStd::vector<AZStd::pair<AZ::Data::AssetId, AZ::Data::Asset<AZ::RPI::ShaderAsset>>> CompileInMemoryShaders(
        const AZStd::vector<InMemoryShaderRequest>& requests);
} // namespace MaterialCanvas
