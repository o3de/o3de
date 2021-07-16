/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Pass/PassRequest.h>

namespace AZ
{
    namespace RPI
    {
        void PassRequest::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PassRequest>()
                    ->Version(3)
                    ->Field("Name", &PassRequest::m_passName)
                    ->Field("TemplateName", &PassRequest::m_templateName)
                    ->Field("Connections", &PassRequest::m_connections)
                    ->Field("PassData", &PassRequest::m_passData)
                    ->Field("Enabled", &PassRequest::m_passEnabled)
                    ->Field("ExecuteAfter", &PassRequest::m_executeAfterPasses)
                    ->Field("ExecuteBefore", &PassRequest::m_executeBeforePasses)
                    ->Field("ImageAttachments", &PassRequest::m_imageAttachmentOverrides)
                    ->Field("BufferAttachments", &PassRequest::m_bufferAttachmentOverrides)
                    ;
            }
        }

        void PassRequest::AddInputConnection(PassConnection inputConnection)
        {
            m_connections.push_back(inputConnection);
        }
    }   // namespace RPI
}   // namespace AZ
