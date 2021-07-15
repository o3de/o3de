/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/LuxCore/LuxCoreTexturePass.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#include <Atom/RHI/RHISystemInterface.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<LuxCoreTexturePass> LuxCoreTexturePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<LuxCoreTexturePass> pass = aznew LuxCoreTexturePass(descriptor);
            return pass;
        }

        LuxCoreTexturePass::LuxCoreTexturePass(const RPI::PassDescriptor& descriptor)
            : ParentPass(descriptor)
        {
            RPI::PassSystemInterface* passSystem = RPI::PassSystemInterface::Get();

            // Create render target pass
            RPI::PassRequest request;
            request.m_templateName = "RenderTextureTemplate";
            request.m_passName = "RenderTarget";
            m_renderTargetPass = passSystem->CreatePassFromRequest(&request);
            AZ_Assert(m_renderTargetPass, "render target pass is invalid");

            // Create readback
            m_readback = AZStd::make_shared<AZ::RPI::AttachmentReadback>(AZ::RHI::ScopeId{ Uuid::CreateRandom().ToString<AZStd::string>() });
        }
        
        LuxCoreTexturePass::~LuxCoreTexturePass()
        {
            m_renderTargetPass = nullptr;
            m_readback = nullptr;
        }

        void LuxCoreTexturePass::SetSourceTexture(AZ::Data::Instance<AZ::RPI::Image> image, RHI::Format format)
        {
            static_cast<RenderTexturePass*>(m_renderTargetPass.get())->SetPassSrgImage(image, format);
        }

        void LuxCoreTexturePass::CreateChildPassesInternal()
        {
            AddChild(m_renderTargetPass);
        }

        void LuxCoreTexturePass::BuildInternal()
        {
            ParentPass::BuildInternal();
        }

        void LuxCoreTexturePass::FrameBeginInternal(FramePrepareParams params)
        {
            if (!m_attachmentReadbackComplete && m_readback != nullptr)
            {
                // Set up read back attachment before children prepare
                if (m_readback->IsReady())
                {
                    if (m_renderTargetPass)
                    {
                        m_attachmentReadbackComplete = m_renderTargetPass->ReadbackAttachment(m_readback, AZ::Name("RenderTargetOutput"));                           
                    }
                }
            }
            ParentPass::FrameBeginInternal(params);
        }

        void LuxCoreTexturePass::SetIsNormalTexture(bool isNormal)
        {
            static_cast<RenderTexturePass*>(m_renderTargetPass.get())->InitShaderVariant(isNormal);
        }

        void LuxCoreTexturePass::SetReadbackCallback(RPI::AttachmentReadback::CallbackFunction callbackFunciton)
        {
            if (m_readback != nullptr)
            {
                m_readback->SetCallback(callbackFunciton);
            }
        }
    }
}
