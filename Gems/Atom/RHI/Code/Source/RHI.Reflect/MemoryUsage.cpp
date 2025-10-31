/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/MemoryUsage.h>

namespace AZ::RHI
{
    HeapMemoryTransfer::HeapMemoryTransfer(const HeapMemoryTransfer& rhs)
    {
        *this = rhs;
    }

    HeapMemoryTransfer& HeapMemoryTransfer::operator=(const HeapMemoryTransfer& rhs)
    {
        m_bytesPerFrame = rhs.m_bytesPerFrame.load();
        m_accumulatedInBytes = rhs.m_accumulatedInBytes;
        return *this;
    }

    HeapMemoryUsage::HeapMemoryUsage(const HeapMemoryUsage& rhs)
    {
        *this = rhs;
    }

    HeapMemoryUsage& HeapMemoryUsage::operator=(const HeapMemoryUsage& rhs)
    {
        m_budgetInBytes = rhs.m_budgetInBytes;
        m_totalResidentInBytes = rhs.m_totalResidentInBytes.load();
        m_usedResidentInBytes = rhs.m_usedResidentInBytes.load();
        m_uniqueAllocationBytes = rhs.m_uniqueAllocationBytes.load();
        m_fragmentation = rhs.m_fragmentation;
        return *this;
    }
}
