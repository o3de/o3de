/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DiffuseGlobalIllumination/DiffuseProbeGridTextureReadback.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGrid.h>

namespace AZ
{
    namespace Render
    {
        DiffuseProbeGridTextureReadback::DiffuseProbeGridTextureReadback(DiffuseProbeGrid* diffuseProbeGrid)
            : m_diffuseProbeGrid(diffuseProbeGrid)
        {
        }

        void DiffuseProbeGridTextureReadback::BeginTextureReadback(DiffuseProbeGridBakeTexturesCallback callback)
        {
            AZ_Assert(m_readbackState == DiffuseProbeGridReadbackState::Idle, "DiffuseProbeGridTextureReadback is already processing a readback request");

            m_callback = callback;
            m_readbackState = DiffuseProbeGridReadbackState::Irradiance;
        }

        void DiffuseProbeGridTextureReadback::Update(const AZ::Name& passName)
        {
            if (m_readbackState == DiffuseProbeGridReadbackState::Idle || m_readbackState == DiffuseProbeGridReadbackState::Complete)
            {
                return;
            }

            if (m_attachmentReadback.get() && m_attachmentReadback->GetReadbackState() > RPI::AttachmentReadback::ReadbackState::Idle)
            {
                // still processing previous request
                return;
            }

            AZStd::string readbackName = AZStd::string::format("DiffuseProbeGridReadback_%s", passName.GetCStr());
            RHI::ImageDescriptor descriptor;
            RHI::AttachmentId attachmentId;
            RPI::AttachmentReadback::CallbackFunction callbackFunction;

            switch (m_readbackState)
            {
            case DiffuseProbeGridReadbackState::Irradiance:
                descriptor = m_diffuseProbeGrid->GetIrradianceImage()->GetDescriptor();
                attachmentId = m_diffuseProbeGrid->GetIrradianceImageAttachmentId();
                callbackFunction = [this](const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult)
                {
                    m_irradianceReadbackResult = readbackResult;
                    m_readbackState = DiffuseProbeGridReadbackState::Distance;
                };
                break;
            case DiffuseProbeGridReadbackState::Distance:
                descriptor = m_diffuseProbeGrid->GetDistanceImage()->GetDescriptor();
                attachmentId = m_diffuseProbeGrid->GetDistanceImageAttachmentId();
                callbackFunction = [this](const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult)
                {
                    m_distanceReadbackResult = readbackResult;
                    m_readbackState = DiffuseProbeGridReadbackState::Relocation;
                };
                break;
            case DiffuseProbeGridReadbackState::Relocation:
                descriptor = m_diffuseProbeGrid->GetRelocationImage()->GetDescriptor();
                attachmentId = m_diffuseProbeGrid->GetRelocationImageAttachmentId();
                callbackFunction = [this](const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult)
                {
                    m_relocationReadbackResult = readbackResult;
                    m_readbackState = DiffuseProbeGridReadbackState::Classification;
                };
                break;
            case DiffuseProbeGridReadbackState::Classification:
                descriptor = m_diffuseProbeGrid->GetClassificationImage()->GetDescriptor();
                attachmentId = m_diffuseProbeGrid->GetClassificationImageAttachmentId();
                callbackFunction = [this](const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult)
                {
                    m_classificationReadbackResult = readbackResult;
                    m_readbackState = DiffuseProbeGridReadbackState::Complete;
                };
                break;
            default:
                AZ_Assert(false, "Unknown readback state");
            }

            m_attachmentReadback = AZStd::make_shared<AZ::RPI::AttachmentReadback>(AZ::RHI::ScopeId{ "DiffuseProbeGridTextureReadBack" });
            m_attachmentReadback->SetCallback(callbackFunction);

            AZ::RPI::PassAttachment passAttachment;
            passAttachment.m_descriptor = descriptor;
            passAttachment.m_path = attachmentId;
            passAttachment.m_name = readbackName;
            passAttachment.m_lifetime = RHI::AttachmentLifetimeType::Imported;

            m_attachmentReadback->ReadPassAttachment(&passAttachment, AZ::Name(readbackName));
        }

        void DiffuseProbeGridTextureReadback::FrameBegin(AZ::RPI::Pass::FramePrepareParams& params)
        {
            if (m_readbackState == DiffuseProbeGridReadbackState::Idle)
            {
                return;
            }

            if (!m_attachmentReadback.get())
            {
                return;
            }

            if (m_readbackState == DiffuseProbeGridReadbackState::Complete)
            {
                // readback of all textures is complete, invoke callback and return to Idle state
                m_callback(
                    { m_irradianceReadbackResult.m_dataBuffer, m_irradianceReadbackResult.m_imageDescriptor.m_format, m_irradianceReadbackResult.m_imageDescriptor.m_size },
                    { m_distanceReadbackResult.m_dataBuffer, m_distanceReadbackResult.m_imageDescriptor.m_format, m_distanceReadbackResult.m_imageDescriptor.m_size },
                    { m_relocationReadbackResult.m_dataBuffer, m_relocationReadbackResult.m_imageDescriptor.m_format, m_relocationReadbackResult.m_imageDescriptor.m_size },
                    { m_classificationReadbackResult.m_dataBuffer, m_classificationReadbackResult.m_imageDescriptor.m_format, m_classificationReadbackResult.m_imageDescriptor.m_size });

                m_readbackState = DiffuseProbeGridReadbackState::Idle;
                m_attachmentReadback.reset();
                return;
            }

            m_attachmentReadback->FrameBegin(params);
        }
    }   // namespace Render
}   // namespace AZ
