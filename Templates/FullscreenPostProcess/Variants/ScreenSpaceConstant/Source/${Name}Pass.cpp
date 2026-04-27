/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "${Name}Pass.h"

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

        // Cache SRG indices here. Example -- uncomment together with the
        // matching m_tintIndex member in ${Name}Pass.h AND a 'float4 m_tint'
        // entry in the azsl PassSrg:
        //
        //   if (m_shaderResourceGroup)
        //   {
        //       m_tintIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(
        //           AZ::Name("m_tint"));
        //   }
    }

    void ${SanitizedCppName}Pass::FrameBeginInternal(FramePrepareParams params)
    {
        // Push global per-frame uniform values. Because the ScreenSpaceConstant
        // variant has no per-area / per-component config, values come from
        // CVars, hardcoded constants, or a singleton interface you wire up
        // separately. If you want artist-controllable per-area parameters,
        // regenerate this template with integration_mode = "PostVolume".
        //
        //   if (m_shaderResourceGroup && m_tintIndex.IsValid())
        //   {
        //       m_shaderResourceGroup->SetConstant(m_tintIndex, AZ::Vector4(1, 0, 0, 1));
        //   }

        AZ::RPI::FullscreenTrianglePass::FrameBeginInternal(params);
    }

} // namespace ${GemName}
