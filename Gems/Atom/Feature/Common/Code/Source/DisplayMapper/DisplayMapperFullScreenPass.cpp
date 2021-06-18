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

#include <Atom/Feature/DisplayMapper/DisplayMapperFullScreenPass.h>
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
