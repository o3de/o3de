/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/Handle.h>

#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>

namespace AZ
{
    namespace RPI
    {
        void PassTemplate::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PassTemplate>()
                    ->Version(3)
                    ->Field("Name", &PassTemplate::m_name)
                    ->Field("PassClass", &PassTemplate::m_passClass)
                    ->Field("Slots", &PassTemplate::m_slots)
                    ->Field("ImageAttachments", &PassTemplate::m_imageAttachments)
                    ->Field("BufferAttachments", &PassTemplate::m_bufferAttachments)
                    ->Field("Connections", &PassTemplate::m_connections)
                    ->Field("FallbackConnections", &PassTemplate::m_fallbackConnections)
                    ->Field("PassRequests", &PassTemplate::m_passRequests)
                    ->Field("PassData", &PassTemplate::m_passData)
                    ;
            }
        }

        const PassRequest* PassTemplate::FindPassRequest(const Name& passName) const
        {
            for (const PassRequest& request : m_passRequests)
            {
                if (request.m_passName == passName)
                {
                    return &request;
                }
            }
            return nullptr;
        }

        bool PassTemplate::AttachmentFitsSlot(const RHI::UnifiedAttachmentDescriptor& attachmentDesc, Name slotName) const
        {
            // Check input slots
            for (const PassSlot& slot : m_slots)
            {
                if (slotName == slot.m_name)
                {
                    return (slot.AcceptsFormat(attachmentDesc) &&
                            slot.AcceptsDimension(attachmentDesc));
                }
            }
            return false;
        }

        void PassTemplate::AddSlot(PassSlot passSlot)
        {
            m_slots.push_back(passSlot);
        }

        void PassTemplate::AddOutputConnection(PassConnection connection)
        {
            m_connections.push_back(connection);
        }

        void PassTemplate::AddImageAttachment(PassImageAttachmentDesc imageAttachment)
        {
            m_imageAttachments.push_back(imageAttachment);
        }

        void PassTemplate::AddBufferAttachment(PassBufferAttachmentDesc bufferAttachment)
        {
            m_bufferAttachments.push_back(bufferAttachment);
        }

        void PassTemplate::AddPassRequest(PassRequest passRequest)
        {
            m_passRequests.push_back(passRequest);
        }

    }   // namespace RPI
}   // namespace AZ
