/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/MemoryUsage.h>

namespace AZ
{
    namespace RHI
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
            m_reservedInBytes = rhs.m_reservedInBytes.load();
            m_residentInBytes = rhs.m_residentInBytes.load();
            return *this;
        }
    }
}
