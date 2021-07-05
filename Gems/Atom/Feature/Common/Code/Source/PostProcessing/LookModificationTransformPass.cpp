/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>

#include <PostProcessing/LookModificationTransformPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcessing/BlendColorGradingLutsPass.h>
#include <PostProcessing/LookModificationCompositePass.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<LookModificationPass> LookModificationPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<LookModificationPass> pass = aznew LookModificationPass(descriptor);
            return pass;
        }

        LookModificationPass::LookModificationPass(const RPI::PassDescriptor& descriptor)
            : RPI::ParentPass(descriptor)
        {
            AzFramework::NativeWindowHandle windowHandle = nullptr;
            AzFramework::WindowSystemRequestBus::BroadcastResult(
                windowHandle,
                &AzFramework::WindowSystemRequestBus::Events::GetDefaultWindowHandle);
        }

        void LookModificationPass::BuildInternal()
        {
            m_swapChainAttachmentBinding = FindAttachmentBinding(Name("SwapChainOutput"));
            ParentPass::BuildInternal();
        }

        void LookModificationPass::FrameBeginInternal([[maybe_unused]] FramePrepareParams params)
        {
            // Get swap chain format
            RHI::Format swapChainFormat = RHI::Format::Unknown;
            if (m_swapChainAttachmentBinding && m_swapChainAttachmentBinding->m_attachment)
            {
                swapChainFormat = m_swapChainAttachmentBinding->m_attachment->GetTransientImageDescriptor().m_imageDescriptor.m_format;
            }

            if (m_displayBufferFormat != swapChainFormat)
            {
                m_displayBufferFormat = swapChainFormat;
                m_outputDeviceTransformType = AcesDisplayMapperFeatureProcessor::GetOutputDeviceTransformType(m_displayBufferFormat);
                m_shaperParams = GetAcesShaperParameters(m_outputDeviceTransformType);

                // Update the children passes
                for (const AZ::RPI::Ptr<Pass>& child : m_children)
                {
                    BlendColorGradingLutsPass* blendPass = azrtti_cast<BlendColorGradingLutsPass*>(child.get());
                    if (blendPass)
                    {
                        blendPass->SetShaperParameters(m_shaperParams);
                        continue;
                    }
                    LookModificationCompositePass* compositePass = azrtti_cast<LookModificationCompositePass*>(child.get());
                    if (compositePass)
                    {
                        compositePass->SetShaperParameters(m_shaperParams);
                        continue;
                    }
                }
            }
            ParentPass::FrameBeginInternal(params);
        }
    }   // namespace Render
}   // namespace AZ
