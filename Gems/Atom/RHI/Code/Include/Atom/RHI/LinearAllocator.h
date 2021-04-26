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

#include <Atom/RHI/Allocator.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * A linear allocator where each allocation is a simple increment to the cursor. Garbage collection
         * controls when the cursor resets back to the beginning.
         */
        class LinearAllocator final
            : public Allocator
        {
        public:
            struct Descriptor : Allocator::Descriptor
            {
                /// No additional members.
            };

            AZ_CLASS_ALLOCATOR(LinearAllocator, AZ::SystemAllocator, 0);

            LinearAllocator() = default;

            void Init(const Descriptor& descriptor);

            //////////////////////////////////////////////////////////////////////////
            // Allocator
            void Shutdown() override;
            VirtualAddress Allocate(size_t byteCount, size_t byteAlignment) override;
            void DeAllocate(VirtualAddress allocation) override;
            void GarbageCollect() override;
            void GarbageCollectForce() override;
            size_t GetAllocatedByteCount() const override;
            const Descriptor& GetDescriptor() const override;
            //////////////////////////////////////////////////////////////////////////

            inline void SetBaseAddress(VirtualAddress address)
            {
                m_descriptor.m_addressBase = address;
            }

        private:
            Descriptor m_descriptor;
            size_t m_byteOffsetCurrent = 0;
            size_t m_garbageCollectIteration = 0;
        };
    }
}
