/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Pass/RasterPassData.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Passes/HairShortCutGeometryShadingPass.h>
#include <Rendering/HairRenderObject.h>
#include <Rendering/HairFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            HairShortCutGeometryShadingPass::HairShortCutGeometryShadingPass(const RPI::PassDescriptor& descriptor)
                : HairGeometryRasterPass(descriptor)
            {
                o_enableShadows = AZ::Name("o_enableShadows");
                o_enableDirectionalLights = AZ::Name("o_enableDirectionalLights");
                o_enablePunctualLights = AZ::Name("o_enablePunctualLights");
                o_enableAreaLights = AZ::Name("o_enableAreaLights");
                o_enableIBL = AZ::Name("o_enableIBL");
                o_hairLightingModel = AZ::Name("o_hairLightingModel");
                o_enableMarschner_R = AZ::Name("o_enableMarschner_R");
                o_enableMarschner_TRT = AZ::Name("o_enableMarschner_TRT");
                o_enableMarschner_TT = AZ::Name("o_enableMarschner_TT");
                o_enableLongtitudeCoeff = AZ::Name("o_enableLongtitudeCoeff");
                o_enableAzimuthCoeff = AZ::Name("o_enableAzimuthCoeff");

                SetShaderPath("Shaders/hairshortcutgeometryshading.azshader");
            }

            RPI::Ptr<HairShortCutGeometryShadingPass> HairShortCutGeometryShadingPass::Create(const RPI::PassDescriptor& descriptor)
            {
                RPI::Ptr<HairShortCutGeometryShadingPass> pass = aznew HairShortCutGeometryShadingPass(descriptor);
                return pass;
            }

            void HairShortCutGeometryShadingPass::UpdateGlobalShaderOptions()
            {
                RPI::ShaderOptionGroup shaderOption = m_shader->CreateShaderOptionGroup();

                m_featureProcessor->GetHairGlobalSettings(m_hairGlobalSettings);

                shaderOption.SetValue(o_enableShadows, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableShadows });
                shaderOption.SetValue(o_enableDirectionalLights, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableDirectionalLights });
                shaderOption.SetValue(o_enablePunctualLights, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enablePunctualLights });
                shaderOption.SetValue(o_enableAreaLights, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableAreaLights });
                shaderOption.SetValue(o_enableIBL, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableIBL });
                shaderOption.SetValue(o_hairLightingModel, AZ::Name{ "HairLightingModel::" + AZStd::string(HairLightingModelNamespace::ToString(m_hairGlobalSettings.m_hairLightingModel)) });
                shaderOption.SetValue(o_enableMarschner_R, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableMarschner_R });
                shaderOption.SetValue(o_enableMarschner_TRT, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableMarschner_TRT });
                shaderOption.SetValue(o_enableMarschner_TT, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableMarschner_TT });
                shaderOption.SetValue(o_enableLongtitudeCoeff, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableLongtitudeCoeff });
                shaderOption.SetValue(o_enableAzimuthCoeff, AZ::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableAzimuthCoeff });
                shaderOption.SetUnspecifiedToDefaultValues();
                if (shaderOption.GetShaderVariantId() != m_currentShaderVariantId)
                {
                    UpdateShaderOptions(shaderOption.GetShaderVariantId());
                    m_featureProcessor->ForceRebuildRenderData();
                }
            }

            void HairShortCutGeometryShadingPass::CompileResources(const RHI::FrameGraphCompileContext& context)
            {
                if (!m_shaderResourceGroup || !AcquireFeatureProcessor())
                {
                    AZ_Error("Hair Gem", m_shaderResourceGroup, "HairShortCutGeometryShadingPass: missing Srg or no feature processor yet");
                    return;     // no error message due to FP - initialization not complete yet, wait for the next frame
                }

                UpdateGlobalShaderOptions();

                // Update the material array constant buffer within the per pass srg
                SrgBufferDescriptor descriptor = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::Constant, RHI::Format::Unknown,
                    sizeof(AMD::TressFXShadeParams), 1,
                    Name{ "HairMaterialsArray" }, Name{ "m_hairParams" }, 0, 0
                );

                m_featureProcessor->GetMaterialsArray().UpdateGPUData(m_shaderResourceGroup, descriptor);

                // Compilation of remaining srgs will be done by the parent class 
                HairGeometryRasterPass::CompileResources(context);
            }

            void HairShortCutGeometryShadingPass::BuildInternal()
            {
                HairGeometryRasterPass::BuildInternal(); // change this to call parent if the method exists

                if (!AcquireFeatureProcessor())
                {
                    return;
                }
            }

            void HairShortCutGeometryShadingPass::InitializeInternal()
            {
                HairGeometryRasterPass::InitializeInternal();
                LoadShaderAndPipelineState();
            }

        } // namespace Hair
    }   // namespace Render
}   // namespace AZ
