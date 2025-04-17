/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/CommandList.h>
#include <PostProcessing/DepthOfFieldWriteFocusDepthFromGpuPass.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DepthOfFieldWriteFocusDepthFromGpuPass> DepthOfFieldWriteFocusDepthFromGpuPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DepthOfFieldWriteFocusDepthFromGpuPass> pass = aznew DepthOfFieldWriteFocusDepthFromGpuPass(descriptor);
            return pass;
        }

        DepthOfFieldWriteFocusDepthFromGpuPass::DepthOfFieldWriteFocusDepthFromGpuPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        void DepthOfFieldWriteFocusDepthFromGpuPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "%s has a null shader resource group when calling Compile.", GetPathName().GetCStr());

            m_shaderResourceGroup->SetConstant(m_autoFocusScreenPositionIndex, m_autoFocusScreenPosition);
            m_shaderResourceGroup->SetBufferView(
                m_autoFocusDataBufferIndex, m_bufferRef->GetBufferView());

            BindPassSrg(context, m_shaderResourceGroup);
            m_shaderResourceGroup->Compile();
        }

        void DepthOfFieldWriteFocusDepthFromGpuPass::SetScreenPosition(const AZ::Vector2& screenPosition)
        {
            m_autoFocusScreenPosition = screenPosition;
        }

        void DepthOfFieldWriteFocusDepthFromGpuPass::SetBufferRef(RPI::Ptr<RPI::Buffer> bufferRef)
        {
            m_bufferRef = bufferRef;
        }

        void DepthOfFieldWriteFocusDepthFromGpuPass::BuildInternal()
        {
            AZ_Assert(m_bufferRef != nullptr, "%s has a null buffer when calling BuildInternal.", GetPathName().GetCStr());

            AttachBufferToSlot(Name("DofDepthInputOutput"), m_bufferRef);
        }

    }   // namespace Render
}   // namespace AZ
