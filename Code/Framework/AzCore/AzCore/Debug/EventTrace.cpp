/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Debug/EventTraceDrillerBus.h>
#include <AzCore/std/time.h>
#include <AzCore/std/parallel/thread.h>

namespace AZ::Debug
{
    EventTrace::ScopedSlice::ScopedSlice(const char* name, const char* category)
        : m_Name(name)
        , m_Category(category)
        , m_Time(AZStd::GetTimeNowMicroSecond())
    {
    }

    EventTrace::ScopedSlice::~ScopedSlice()
    {
        EventTraceDrillerBus::TryQueueBroadcast(
            &EventTraceDrillerInterface::RecordSlice, m_Name, m_Category, AZStd::this_thread::get_id(), m_Time,
            (uint32_t)(AZStd::GetTimeNowMicroSecond() - m_Time));
    }
} // namespace AZ::Debug
