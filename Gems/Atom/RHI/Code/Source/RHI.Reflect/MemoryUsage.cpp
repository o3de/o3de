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
