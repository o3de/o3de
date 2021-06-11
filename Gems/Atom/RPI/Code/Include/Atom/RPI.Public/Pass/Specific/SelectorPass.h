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

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/ScopeProducerFunction.h>
#include <Atom/RHI/ShaderResourceGroup.h>

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/SwapChainDescriptor.h>

#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>

#include <AzFramework/Windowing/WindowBus.h>

namespace AZ
{
    namespace RPI
    {
        //! A pass provide the ability to pass through input attachments to output attachments
        //! For example, it can choose one of the input attachments as output atachments. 
        class SelectorPass final
            : public Pass
        {
        public:
            AZ_RTTI(SelectorPass, "{A6BCB7A5-EF09-4863-BC8C-C84655067984}", Pass);
            AZ_CLASS_ALLOCATOR(SelectorPass, SystemAllocator, 0);

            static Ptr<SelectorPass> Create(const PassDescriptor& descriptor);
            ~SelectorPass() = default;

            //! Connect the attachment from the inputSlot to the specified outputSlot
            void Connect(const AZ::Name& inputSlot, const AZ::Name& outputSlot);
            void Connect(uint32_t inputSlotIndex, uint32_t outputSlotIndex);

        protected:
            SelectorPass(const PassDescriptor& descriptor);

            // Pass behavior overrides
            void BuildInternal() final;

            // the input slot index each output slot connect to
            AZStd::vector<uint32_t> m_connections;
        };

    }   // namespace RPI
}   // namespace AZ
