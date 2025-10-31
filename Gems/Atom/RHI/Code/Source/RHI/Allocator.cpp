/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Allocator.h>

namespace AZ::RHI
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
