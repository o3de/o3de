/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectionScreenSpaceCompositePass.h"
#include "ReflectionScreenSpaceBlurPass.h"
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<ReflectionScreenSpaceCompositePass> ReflectionScreenSpaceCompositePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<ReflectionScreenSpaceCompositePass> pass = aznew ReflectionScreenSpaceCompositePass(descriptor);
            return AZStd::move(pass);
        }

        ReflectionScreenSpaceCompositePass::ReflectionScreenSpaceCompositePass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        bool ReflectionScreenSpaceCompositePass::IsEnabled() const
        {
            // delay for a few frames to ensure that the previous frame texture is populated
            static const uint32_t FrameDelay = 10;
            if (m_frameDelayCount < FrameDelay)
            {
                m_frameDelayCount++;
                return false;
            }

            return true;
        }

        void ReflectionScreenSpaceCompositePass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            if (!m_shaderResourceGroup)
            {
                return;
            }

            auto constantIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name("m_maxMipLevel"));
            m_shaderResourceGroup->SetConstant(constantIndex, ReflectionScreenSpaceBlurPass::NumMipLevels - 1);

            FullscreenTrianglePass::CompileResources(context);
        }
    }   // namespace RPI
}   // namespace AZ
