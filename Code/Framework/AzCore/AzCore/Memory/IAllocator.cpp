/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Memory/IAllocator.h>

namespace AZ
{
    IAllocator::IAllocator(IAllocatorAllocate* allocationSource)
        : m_allocationSource(allocationSource)
        , m_originalAllocationSource(allocationSource)
    {
    }

    IAllocator::~IAllocator()
    {
    }

    void IAllocator::SetAllocationSource(IAllocatorAllocate* allocationSource)
    {
        m_allocationSource = allocationSource;
    }

    void IAllocator::ResetAllocationSource()
    {
        m_allocationSource = m_originalAllocationSource;
    }
}
