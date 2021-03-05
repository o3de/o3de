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
#pragma once

#include <Atom/RHI.Reflect/AttachmentId.h>

#include <Atom/RPI.Reflect/Pass/PassRequest.h>

#include <AtomCore/std/containers/array_view.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    namespace RPI
    {
        //! Used to data drive passes in the pass system. Contains lists of input and
        //! output attachment slots as well as requests to instantiate child passes.
        class PassTemplate final
        {
        public:
            friend class PassAsset;

            AZ_RTTI(PassTemplate, "{BF485F6D-02EC-4BAD-94BA-519248F59D14}");
            AZ_CLASS_ALLOCATOR(PassTemplate, SystemAllocator, 0);

            static void Reflect(ReflectContext* context);

            ~PassTemplate() = default;

            AZStd::shared_ptr<PassTemplate> Clone() const {
                return AZStd::make_shared<PassTemplate>(*this);
            }

            AZStd::unique_ptr<PassTemplate> CloneUnique() const {
                return AZStd::make_unique<PassTemplate>(*this);
            }

            //! Returns whether the given attachment matches the restrictions for the slot
            bool AttachmentFitsSlot(const RHI::UnifiedAttachmentDescriptor& attachmentDesc, Name slotName) const;

            //! Find a pass request by name in m_passRequests
            const PassRequest* FindPassRequest(const Name& passName) const;

            //! Adds a slot to the PassTemplate
            void AddSlot(PassSlot passSlot);

            //! Adds an output connection to the PassTemplate
            void AddOutputConnection(PassConnection connection);

            //! Adds an image descriptor to the PassTemplate
            void AddImageAttachment(PassImageAttachmentDesc imageAttachment);

            //! Adds an image descriptor to the PassTemplate
            void AddBufferAttachment(PassBufferAttachmentDesc bufferAttachment);

            //! Adds a pass request to the PassTemplate
            void AddPassRequest(PassRequest passRequest);

            Name m_name;

            //! Name of the pass class to instantiate
            Name m_passClass;

            //! Lists of inputs, outputs and input/outputs
            PassSlotList m_slots;

            //! Connections for the Pass's outputs. Input connections are specified by PassRequests.
            //! Output connections will often point to attachments owned by the pass, whereas input
            //! connections will be hooked up to other passes.
            PassConnectionList m_outputConnections;

            //! Fallback connections for the Pass's outputs. These connections will hook up to inputs
            //! of the pass and provide a fallback attachment for when the pass is disabled.
            PassFallbackConnectionList m_fallbackConnections;

            //! List of descriptors for the image attachments the Pass will own
            PassImageAttachmentDescList m_imageAttachments;

            //! List of descriptors for the buffer attachments the Pass will own
            PassBufferAttachmentDescList m_bufferAttachments;

            //! List of requests to create child passes
            PassRequestList m_passRequests;

            //! Optional data to be used during pass initialization
            AZStd::shared_ptr<PassData> m_passData = nullptr;
        };
    }   // namespace RPI
}   // namespace AZ
