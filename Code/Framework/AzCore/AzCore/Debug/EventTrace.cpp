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

#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Debug/EventTraceDrillerBus.h>
#include <AzCore/std/time.h>
#include <AzCore/std/parallel/thread.h>

namespace AZ
{
    namespace Debug
    {
        EventTrace::ScopedSlice::ScopedSlice(const char* name, const char* category)
            : m_Name(name)
            , m_Category(category)
            , m_Time(AZStd::GetTimeNowMicroSecond())
        {}

        EventTrace::ScopedSlice::~ScopedSlice()
        {
            EventTraceDrillerBus::TryQueueBroadcast(&EventTraceDrillerInterface::RecordSlice, m_Name, m_Category, AZStd::this_thread::get_id(), m_Time, (uint32_t)(AZStd::GetTimeNowMicroSecond() - m_Time));
        }
    }
}
