/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

            m_remainingInitializationFrames = DefaultNumInitializationFrames;
            m_readbackState = DiffuseProbeGridReadbackState::Initializing;
        }

        void DiffuseProbeGridTextureReadback::Update(const AZ::Name& passName)
        {
            if (m_readbackState == DiffuseProbeGridReadbackState::Idle || m_readbackState == DiffuseProbeGridReadbackState::Complete)
            {
                return;
            }

            if (m_readbackState == DiffuseProbeGridReadbackState::Initializing)
            {
                if (m_remainingInitializationFrames > 0)
                {
                    // still in the initialization state to allow the irradiance textures to settle, decrement the frame count
                    m_remainingInitializationFrames--;
                    return;
                }

                // settling complete, move to the irradiance readback state to begin the readback process
                m_readbackState = DiffuseProbeGridReadbackState::Irradiance;
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
                    m_readbackState = DiffuseProbeGridReadbackState::ProbeData;
                };
                break;
            case DiffuseProbeGridReadbackState::ProbeData:
                descriptor = m_diffuseProbeGrid->GetProbeDataImage()->GetDescriptor();
                attachmentId = m_diffuseProbeGrid->GetProbeDataImageAttachmentId();
                callbackFunction = [this](const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult)
                {
                    m_probeDataReadbackResult = readbackResult;
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
                    { m_probeDataReadbackResult.m_dataBuffer, m_probeDataReadbackResult.m_imageDescriptor.m_format, m_probeDataReadbackResult.m_imageDescriptor.m_size });

                m_readbackState = DiffuseProbeGridReadbackState::Idle;
                m_attachmentReadback.reset();
                return;
            }

            m_attachmentReadback->FrameBegin(params);
        }
    }   // namespace Render
}   // namespace AZ
