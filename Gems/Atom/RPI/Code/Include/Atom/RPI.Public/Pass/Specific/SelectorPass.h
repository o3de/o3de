/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/ScopeProducerFunction.h>

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/SwapChainDescriptor.h>

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>

#include <AzFramework/Windowing/WindowBus.h>

namespace AZ
{
    namespace RPI
    {
        //! A pass provide the ability to pass through input attachments to output attachments
        //! For example, it can choose one of the input attachments as output atachments. 
        class ATOM_RPI_PUBLIC_API SelectorPass final
            : public Pass
        {
        public:
            AZ_RTTI(SelectorPass, "{A6BCB7A5-EF09-4863-BC8C-C84655067984}", Pass);
            AZ_CLASS_ALLOCATOR(SelectorPass, SystemAllocator);

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
