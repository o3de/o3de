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
