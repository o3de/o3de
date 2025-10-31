/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DisplayMapper/DisplayMapperFullScreenPass.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>

#include <Atom/RHI/Factory.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DisplayMapperFullScreenPass> DisplayMapperFullScreenPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DisplayMapperFullScreenPass> pass = aznew DisplayMapperFullScreenPass(descriptor);
            return pass;
        }

        DisplayMapperFullScreenPass::DisplayMapperFullScreenPass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        DisplayMapperFullScreenPass::~DisplayMapperFullScreenPass()
        {
        }

        void DisplayMapperFullScreenPass::BuildInternal()
        {
            RPI::PassConnection inConnection;
            inConnection.m_localSlot = InputAttachmentName;
            inConnection.m_attachmentRef.m_pass = m_inputReferencePassName;
            inConnection.m_attachmentRef.m_attachment = m_inputReferenceAttachmentName;
            ProcessConnection(inConnection);
        }

        void DisplayMapperFullScreenPass::SetInputReferencePassName(const Name& passName)
        {
            m_inputReferencePassName = passName;
        }

        void DisplayMapperFullScreenPass::SetInputReferenceAttachmentName(const Name& attachmentName)
        {
            m_inputReferenceAttachmentName = attachmentName;
        }

    }   // namespace Render
}   // namespace AZ
