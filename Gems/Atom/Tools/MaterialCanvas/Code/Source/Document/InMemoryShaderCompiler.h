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
    //! Spike: builds the reflection half of a shader in process, without the Asset Processor, to measure and validate the path.
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

        //! Bytecode size per entry point in compile order; empty where DXC did not run (off Windows).
        AZStd::vector<AZStd::pair<AZStd::string, size_t>> m_stageByteCodeSizes;
        size_t m_dynamicBranchCount = 0;
    };

    //! Runs the spike on a .azsl (MCPP first) or a preprocessed .azslin, logging a per-stage breakdown to "MaterialCanvas".
    InMemoryShaderSpikeResult RunInMemoryShaderSpike(const AZStd::string& preprocessedAzslPath);
    //! Spike: builds the MaterialTypeAsset in process from the intermediate material type, as FinalStage does; measures only.
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

    //! @param materialTypeSourcePath the abstract .materialtype; its intermediate is found via PredictIntermediateMaterialTypeSourcePath.
    InMemoryMaterialSpikeResult RunInMemoryMaterialSpike(const AZStd::string& materialTypeSourcePath);
    //! Builds a MaterialTypeAsset in process from the .materialtype's intermediate; invalid on failure, so fall back to the AP.
    AZ::Data::Asset<AZ::RPI::MaterialTypeAsset> CreateInMemoryMaterialTypeAsset(const AZStd::string& materialTypeSourcePath);

    //! One entry point of a shader: the function name azslc and DXC are pointed at, and which hardware stage it is.
    struct InMemoryShaderEntryPoint
    {
        AZStd::string m_name;
        AZ::RHI::ShaderHardwareStage m_stage = AZ::RHI::ShaderHardwareStage::Invalid;
    };

    //! Compiles AZSL and clones @sourceShaderAsset (same id) with the new bytecode; null if the interface moved. Not on the main thread.
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

    //! Finds the intermediate .azsl/.shader sources of a material type's shaders for in-process recompiling; empty if any are missing.
    AZStd::vector<InMemoryShaderRequest> CollectInMemoryShaderRequests(
        const AZ::Data::Asset<AZ::RPI::MaterialTypeAsset>& materialTypeAsset, const AZStd::string& materialTypeSourcePath);

    //! Recompiles every request and returns the successful shaders keyed by the AssetId they replace; not on the main thread.
    AZStd::vector<AZStd::pair<AZ::Data::AssetId, AZ::Data::Asset<AZ::RPI::ShaderAsset>>> CompileInMemoryShaders(
        const AZStd::vector<InMemoryShaderRequest>& requests);
} // namespace MaterialCanvas
