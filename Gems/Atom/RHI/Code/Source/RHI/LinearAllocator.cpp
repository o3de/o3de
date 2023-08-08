/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Bits.h>
#include <Atom/RHI/LinearAllocator.h>

namespace AZ::RHI
{
    void LinearAllocator::Init(const Descriptor& descriptor)
    {
        Shutdown();
        m_descriptor = descriptor;
    }

    void LinearAllocator::Shutdown()
    {
        GarbageCollectForce();
    }

    void LinearAllocator::GarbageCollectForce()
    {
        m_byteOffsetCurrent = 0;
        m_garbageCollectIteration = 0;
    }

    void LinearAllocator::GarbageCollect()
    {
        if (m_garbageCollectIteration == m_descriptor.m_garbageCollectLatency)
        {
            GarbageCollectForce();
            m_garbageCollectIteration = 0;
        }
        else
        {
            ++m_garbageCollectIteration;
        }
    }

    const LinearAllocator::Descriptor& LinearAllocator::GetDescriptor() const
    {
        return m_descriptor;
    }

    size_t LinearAllocator::GetAllocatedByteCount() const
    {
        return m_byteOffsetCurrent;
    }

    VirtualAddress LinearAllocator::Allocate(size_t byteCount, size_t byteAlignment)
    {
        if (byteCount == 0)
        {
            return VirtualAddress::CreateNull();
        }

        VirtualAddress addressCurrentAligned{ RHI::AlignUp(m_descriptor.m_addressBase.m_ptr + m_byteOffsetCurrent, byteAlignment) };
        size_t byteCountAligned = RHI::AlignUp(byteCount, byteAlignment);
        size_t byteOffsetAligned = addressCurrentAligned.m_ptr - m_descriptor.m_addressBase.m_ptr;
        size_t nextByteAddress = byteOffsetAligned + byteCountAligned;

        if (nextByteAddress > m_descriptor.m_capacityInBytes)
        {
            return VirtualAddress::CreateNull();
        }

        m_byteOffsetCurrent = nextByteAddress;
        return addressCurrentAligned;
    }

    void LinearAllocator::DeAllocate(VirtualAddress offset)
    {
        (void)offset;
    }
}
