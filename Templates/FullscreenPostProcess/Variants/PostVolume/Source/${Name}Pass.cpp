/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "${Name}Pass.h"
#include "${Name}SettingsProviderInterface.h"

#include <AzCore/Interface/Interface.h>

namespace ${GemName}
{
    AZ::RPI::Ptr<${SanitizedCppName}Pass> ${SanitizedCppName}Pass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        return aznew ${SanitizedCppName}Pass(descriptor);
    }

    ${SanitizedCppName}Pass::${SanitizedCppName}Pass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::FullscreenTrianglePass(descriptor)
    {
    }

    void ${SanitizedCppName}Pass::InitializeInternal()
    {
        AZ::RPI::FullscreenTrianglePass::InitializeInternal();

        // Cache SRG indices once the base class has loaded the shader. Example
        // -- uncomment together with the matching members in ${Name}Pass.h
        // AND matching constants in PassSrg in ${Name}.azsl:
        //
        //   if (m_shaderResourceGroup)
        //   {
        //       m_intensityIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(AZ::Name("m_intensity"));
        //       m_tintIndex      = m_shaderResourceGroup->FindShaderInputConstantIndex(AZ::Name("m_tint"));
        //       m_exposureIndex  = m_shaderResourceGroup->FindShaderInputConstantIndex(AZ::Name("m_exposure"));
        //   }
    }

    void ${SanitizedCppName}Pass::FrameBeginInternal(FramePrepareParams params)
    {
        // Pull this frame's parameters from whichever ${Name}SettingsComponent
        // is currently active in the scene. If none is, treat the effect as
        // disabled -- intensity zero. Any per-frame work that depends on the
        // settings should sit in this branch.
        if (auto* provider = AZ::Interface<${SanitizedCppName}SettingsProviderInterface>::Get())
        {
            const auto& settings = provider->GetSettings();
            const float effectiveIntensity = settings.m_enabled ? settings.m_intensity : 0.0f;

            // Push to SRG. Uncomment together with the InitializeInternal
            // index caches and the matching PassSrg constants.
            //
            //   if (m_shaderResourceGroup && m_intensityIndex.IsValid())
            //   {
            //       m_shaderResourceGroup->SetConstant(m_intensityIndex, effectiveIntensity);
            //   }
            //   if (m_shaderResourceGroup && m_tintIndex.IsValid())
            //   {
            //       const AZ::Vector4 tintRgba = settings.m_tint.GetAsVector4();
            //       m_shaderResourceGroup->SetConstant(m_tintIndex, tintRgba);
            //   }
            //   if (m_shaderResourceGroup && m_exposureIndex.IsValid())
            //   {
            //       const float exposureMul = AZStd::pow(2.0f, settings.m_exposureEv);
            //       m_shaderResourceGroup->SetConstant(m_exposureIndex, exposureMul);
            //   }

            (void)effectiveIntensity; // silence unused-var until you uncomment the SetConstant block
        }

        AZ::RPI::FullscreenTrianglePass::FrameBeginInternal(params);
    }

} // namespace ${GemName}
