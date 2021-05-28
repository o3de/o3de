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
#include <Atom/RHI/Allocator.h>

namespace AZ
{
    namespace RHI
    {
        const VirtualAddress VirtualAddress::Null{static_cast<uintptr_t>(-1)};

        VirtualAddress VirtualAddress::CreateFromOffset(uint64_t offset)
        {
            return VirtualAddress{static_cast<uintptr_t>(offset)};
        }

        VirtualAddress VirtualAddress::CreateFromPointer(void* ptr)
        {
            return VirtualAddress{reinterpret_cast<uintptr_t>(ptr)};
        }

        VirtualAddress VirtualAddress::CreateNull()
        {
            return VirtualAddress{};
        }

        VirtualAddress VirtualAddress::CreateZero()
        {
            return VirtualAddress{0};
        }

        VirtualAddress::VirtualAddress()
            : m_ptr{Null.m_ptr}
        {}

        VirtualAddress::VirtualAddress(uintptr_t ptr)
            : m_ptr{ptr}
        {}

        Allocator::Descriptor::Descriptor()
            : m_addressBase{0}
        {}
    }
}
