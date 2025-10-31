/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MatrixUtils.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/Specific/EnvironmentCubeMapPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace RPI
    {
        namespace
        {
            // camera basis vectors for each cubemap face
            const Vector3 CameraBasis[EnvironmentCubeMapPass::NumCubeMapFaces][3] = {
                { Vector3(0.0f, 1.0f, 0.0f), Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) },
                { Vector3(0.0f, -1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) },
                { Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f) },
                { Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, -1.0f, 0.0f) },
                { Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) },
                { Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) },
            };
        } // namespace

        Ptr<EnvironmentCubeMapPass> EnvironmentCubeMapPass::Create(const PassDescriptor& passDescriptor)
        {
            Ptr<EnvironmentCubeMapPass> pass = aznew EnvironmentCubeMapPass(passDescriptor);
            return pass;
        }

        EnvironmentCubeMapPass::EnvironmentCubeMapPass(const PassDescriptor& passDescriptor)
            : ParentPass(passDescriptor)
        {
            // load pass data
            const EnvironmentCubeMapPassData* passData = PassUtils::GetPassData<EnvironmentCubeMapPassData>(passDescriptor);
            if (passData == nullptr)
            {
                AZ_Error("PassSystem", false, "[EnvironmentCubeMapPass '%s']: Trying to construct without valid EnvironmentCubeMapPassData!", GetPathName().GetCStr());
                return;
            }

            m_position = passData->m_position;

            // create the cubemap pipeline as a child of this pass
            PassRequest childRequest;
            childRequest.m_templateName = "EnvironmentCubeMapPipeline";
            childRequest.m_passName = "Child";
            
            PassConnection childInputConnection;
            childInputConnection.m_localSlot = "PipelineOutput";
            childInputConnection.m_attachmentRef.m_pass = "Parent";
            childInputConnection.m_attachmentRef.m_attachment = "Output";
            childRequest.m_connections.emplace_back(childInputConnection);

            PassSystemInterface* passSystem = PassSystemInterface::Get();
            m_childPass = passSystem->CreatePassFromRequest(&childRequest);
            AZ_Assert(m_childPass, "EnvironmentCubeMap child pass is invalid");

            // setup viewport
            m_viewportState.m_minX = 0;
            m_viewportState.m_minY = 0;
            m_viewportState.m_maxX = static_cast<float>(CubeMapFaceSize);
            m_viewportState.m_maxY = static_cast<float>(CubeMapFaceSize);

            // setup scissor
            m_scissorState.m_minX = 0;
            m_scissorState.m_minY = 0;
            m_scissorState.m_maxX = static_cast<int16_t>(CubeMapFaceSize);
            m_scissorState.m_maxY = static_cast<int16_t>(CubeMapFaceSize);

            // create view
            AZ::Name viewName(AZStd::string::format("%s_%s", childRequest.m_templateName.GetCStr(), childRequest.m_passName.GetCStr()));
            m_view = RPI::View::CreateView(viewName, RPI::View::UsageReflectiveCubeMap);
            
            AZ::Matrix3x4 viewTransform;
            const Vector3* basis = &CameraBasis[0][0];
            viewTransform.SetBasisAndTranslation(basis[0], basis[1], basis[2], m_position);
            m_view->SetCameraTransform(viewTransform);
            
            AZ::Matrix4x4 viewToClipMatrix;
            MakePerspectiveFovMatrixRH(viewToClipMatrix, AZ::Constants::HalfPi, 1.0f, 0.1f, 100.0f, true);
            m_view->SetViewToClipMatrix(viewToClipMatrix);
        }

        EnvironmentCubeMapPass::~EnvironmentCubeMapPass()
        {
            for (uint32_t i = 0; i < NumCubeMapFaces; ++i)
            {
                delete [] m_textureData[i];
            }
        }

        void EnvironmentCubeMapPass::SetDefaultView()
        {
            if (m_pipeline)
            {
                m_pipeline->SetDefaultView(m_view);
            }
        }

        void EnvironmentCubeMapPass::CreateChildPassesInternal()
        {            
            AddChild(m_childPass);
        }

        void EnvironmentCubeMapPass::BuildInternal()
        {
            // create output image descriptor
            m_outputImageDesc = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::Color | RHI::ImageBindFlags::CopyRead, CubeMapFaceSize, CubeMapFaceSize, RHI::Format::R16G16B16A16_FLOAT);

            // create output PassAttachment
            m_passAttachment = aznew PassAttachment();
            m_passAttachment->m_name = "Output";
            AZ::Name attachmentPath(AZStd::string::format("%s.%s", GetPathName().GetCStr(), m_passAttachment->m_name.GetCStr()));
            m_passAttachment->m_path = attachmentPath;
            m_passAttachment->m_lifetime = RHI::AttachmentLifetimeType::Transient;
            m_passAttachment->m_descriptor = m_outputImageDesc;

            // create pass attachment binding
            PassAttachmentBinding outputAttachment;
            outputAttachment.m_name = "Output";
            outputAttachment.m_slotType = PassSlotType::InputOutput;
            outputAttachment.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::RenderTarget;
            outputAttachment.SetAttachment(m_passAttachment);

            AddAttachmentBinding(outputAttachment);

            ParentPass::BuildInternal();
        }

        void EnvironmentCubeMapPass::FrameBeginInternal(FramePrepareParams params)
        {
            params.m_scissorState = m_scissorState;
            params.m_viewportState = m_viewportState;

            RHI::FrameGraphAttachmentInterface attachmentDatabase = params.m_frameGraphBuilder->GetAttachmentDatabase();
            attachmentDatabase.CreateTransientImage(RHI::TransientImageDescriptor(m_passAttachment->GetAttachmentId(), m_outputImageDesc));

            m_readBackLock.lock();
            if (!m_attachmentReadback || !m_readBackRequested)
            {
                // create the AttachmentReadback if this is the first time in FramePrepare, or we finished with the last readback
                // in which case we will free the previous one and allocate a new one
                m_attachmentReadback = AZStd::make_shared<AZ::RPI::AttachmentReadback>(AZ::RHI::ScopeId{ "EnvironmentCubeMapReadBack" });
                m_attachmentReadback->SetCallback(AZStd::bind(&EnvironmentCubeMapPass::AttachmentReadbackCallback, this, AZStd::placeholders::_1));
            }
            m_readBackLock.unlock();

            ParentPass::FrameBeginInternal(params);

            // Note: this needs to be after the call to ParentPass::FrameBeginInternal in order to setup the scopes correctly for readback
            m_attachmentReadback->FrameBegin(params);
        }

        void EnvironmentCubeMapPass::FrameEndInternal()
        {
            m_readBackLock.lock();
            if (m_renderFace < NumCubeMapFaces)
            {
                if (!m_readBackRequested)
                {
                    // delay a number of frames before requesting the readback
                    if (m_readBackDelayFrames < NumReadBackDelayFrames)
                    {
                        m_readBackDelayFrames++;
                    }
                    else
                    {
                        m_readBackRequested = true;
                        AZStd::string readbackName = AZStd::string::format("%s_%s", m_passAttachment->GetAttachmentId().GetCStr(), GetName().GetCStr());
                        m_attachmentReadback->ReadPassAttachment(m_passAttachment.get(), AZ::Name(readbackName));
                    }
                }

                // set the appropriate render camera transform for the next frame
                AZ::Matrix3x4 viewTransform;
                const Vector3* basis = &CameraBasis[m_renderFace][0];
                viewTransform.SetBasisAndTranslation(basis[0], basis[1], basis[2], m_position);

                m_view->SetCameraTransform(viewTransform);
                m_pipeline->SetDefaultView(m_view);
            }
            m_readBackLock.unlock();

            ParentPass::FrameEndInternal();
        }      

        void EnvironmentCubeMapPass::AttachmentReadbackCallback(const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult)
        {
            RHI::DeviceImageSubresourceLayout imageLayout =
                RHI::GetImageSubresourceLayout(readbackResult.m_imageDescriptor.m_size, readbackResult.m_imageDescriptor.m_format);

            delete [] m_textureData[m_renderFace];

            // copy face texture data
            m_textureData[m_renderFace] = new uint8_t[imageLayout.m_bytesPerImage];
            uint32_t bytesRead = (uint32_t)readbackResult.m_dataBuffer->size();
            memcpy(m_textureData[m_renderFace], readbackResult.m_dataBuffer->data(), bytesRead);

            m_textureFormat = readbackResult.m_imageDescriptor.m_format;

            m_readBackLock.lock();
            {
                // move to the next face
                m_renderFace++;
                m_readBackRequested = false;
                m_readBackDelayFrames = 0;
            }
            m_readBackLock.unlock();
        }

    }   // namespace RPI
}   // namespace AZ
