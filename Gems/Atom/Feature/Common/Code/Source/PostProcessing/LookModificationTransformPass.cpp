/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        void LookModificationPass::BuildAttachmentsInternal()
        {
            m_swapChainAttachmentBinding = FindAttachmentBinding(Name("SwapChainOutput"));
            ParentPass::BuildAttachmentsInternal();
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
