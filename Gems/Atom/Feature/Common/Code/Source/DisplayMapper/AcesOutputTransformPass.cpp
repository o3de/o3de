/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DisplayMapper/AcesOutputTransformPass.h>
#include <ACES/Aces.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/PassFactory.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/View.h>

#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/DevicePipelineState.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<AcesOutputTransformPass> AcesOutputTransformPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<AcesOutputTransformPass> pass = aznew AcesOutputTransformPass(descriptor);
            return pass;
        }

        AcesOutputTransformPass::AcesOutputTransformPass(const RPI::PassDescriptor& descriptor)
            : DisplayMapperFullScreenPass(descriptor)
        {
        }

        void AcesOutputTransformPass::InitializeInternal()
        {
            DisplayMapperFullScreenPass::InitializeInternal();

            AZ_Assert(m_shaderResourceGroup != nullptr, "AcesOutputTransformPass %s has a null shader resource group when calling Init.", GetPathName().GetCStr());

            if (m_shaderResourceGroup != nullptr)
            {
                m_shaderInputColorMatIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_XYZtoDisplayPrimaries" });
                m_shaderInputCinemaLimitsIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_cinemaLimits" });
                m_shaderInputAcesSplineParamsIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_acesSplineParams" });
                m_shaderInputFlagsIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_outputDisplayTransformFlags" });
                m_shaderInputOutputModeIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_outputDisplayTransformMode" });
                m_shaderInputSurroundGammaIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_surroundGamma" });
                m_shaderInputGammaIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_gamma" });
            }
        }

        void AcesOutputTransformPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "AcesOutputTransformPass %s has a null shader resource group when calling FrameBeginInternal.", GetPathName().GetCStr());

            if (m_shaderResourceGroup != nullptr)
            {
                m_shaderResourceGroup->SetConstant(m_shaderInputColorMatIndex, m_displayMapperParameters.m_XYZtoDisplayPrimaries);
                m_shaderResourceGroup->SetConstant(m_shaderInputCinemaLimitsIndex, m_displayMapperParameters.m_cinemaLimits);
                m_shaderResourceGroup->SetConstantRaw(m_shaderInputAcesSplineParamsIndex, &m_displayMapperParameters.m_acesSplineParams, sizeof(SegmentedSplineParamsC9));
                m_shaderResourceGroup->SetConstant(m_shaderInputFlagsIndex, m_displayMapperParameters.m_OutputDisplayTransformFlags);
                m_shaderResourceGroup->SetConstant(m_shaderInputOutputModeIndex, m_displayMapperParameters.m_OutputDisplayTransformMode);
                m_shaderResourceGroup->SetConstant(m_shaderInputSurroundGammaIndex, m_displayMapperParameters.m_surroundGamma);
                m_shaderResourceGroup->SetConstant(m_shaderInputGammaIndex, m_displayMapperParameters.m_gamma);
            }

            BindPassSrg(context, m_shaderResourceGroup);
            m_shaderResourceGroup->Compile();
        }

        void AcesOutputTransformPass::SetDisplayBufferFormat(RHI::Format format)
        {
            if (m_displayBufferFormat != format)
            {
                m_displayBufferFormat = format;
                if (m_displayBufferFormat == RHI::Format::R8G8B8A8_UNORM ||
                    m_displayBufferFormat == RHI::Format::B8G8R8A8_UNORM)
                {
                    AcesDisplayMapperFeatureProcessor::GetAcesDisplayMapperParameters(&m_displayMapperParameters, OutputDeviceTransformType_48Nits);
                }
                else if (m_displayBufferFormat == RHI::Format::R10G10B10A2_UNORM)
                {
                    AcesDisplayMapperFeatureProcessor::GetAcesDisplayMapperParameters(&m_displayMapperParameters, OutputDeviceTransformType_1000Nits);
                }
                else
                {
                    AZ_Assert(false, "Not yet supported.");
                    // To work normally on unsupported environment, initialize the display parameters by OutputDeviceTransformType_48Nits.
                    AcesDisplayMapperFeatureProcessor::GetAcesDisplayMapperParameters(&m_displayMapperParameters, OutputDeviceTransformType_48Nits);
                }
            }

            if (m_acesParameterOverrides.m_overrideDefaults)
            {
                m_displayMapperParameters.m_OutputDisplayTransformFlags = 0;
                if (m_acesParameterOverrides.m_alterSurround)
                {
                    m_displayMapperParameters.m_OutputDisplayTransformFlags |= AcesDisplayMapperFeatureProcessor::AlterSurround;
                }
                if (m_acesParameterOverrides.m_applyDesaturation)
                {
                    m_displayMapperParameters.m_OutputDisplayTransformFlags |= AcesDisplayMapperFeatureProcessor::ApplyDesaturation;
                }
                if (m_acesParameterOverrides.m_applyCATD60toD65)
                {
                    m_displayMapperParameters.m_OutputDisplayTransformFlags |= AcesDisplayMapperFeatureProcessor::ApplyCATD60toD65;
                }

                m_displayMapperParameters.m_cinemaLimits[0] = m_acesParameterOverrides.m_cinemaLimitsBlack;
                m_displayMapperParameters.m_cinemaLimits[1] = m_acesParameterOverrides.m_cinemaLimitsWhite;
                m_displayMapperParameters.m_acesSplineParams.minPoint[0] = m_acesParameterOverrides.m_minPoint;
                m_displayMapperParameters.m_acesSplineParams.midPoint[0] = m_acesParameterOverrides.m_midPoint;
                m_displayMapperParameters.m_acesSplineParams.maxPoint[0] = m_acesParameterOverrides.m_maxPoint;
                m_displayMapperParameters.m_surroundGamma = m_acesParameterOverrides.m_surroundGamma;
                m_displayMapperParameters.m_gamma = m_acesParameterOverrides.m_gamma;
            }
        }

        void AcesOutputTransformPass::SetAcesParameterOverrides(const AcesParameterOverrides& acesParameterOverrides)
        {
            m_acesParameterOverrides = acesParameterOverrides;
        }
    }   // namespace Render
}   // namespace AZ
