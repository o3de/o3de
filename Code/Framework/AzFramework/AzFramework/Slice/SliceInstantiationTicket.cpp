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

#include<AzFramework/Slice/SliceInstantiationTicket.h>

namespace AzFramework
{
    SliceInstantiationTicket::SliceInstantiationTicket(const EntityContextId& contextId, AZ::u64 requestId)
        : m_contextId(contextId)
        , m_requestId(requestId)
    {
    }

    bool SliceInstantiationTicket::operator==(const SliceInstantiationTicket& rhs) const
    {
        return rhs.m_contextId == m_contextId && rhs.m_requestId == m_requestId;
    }

    bool SliceInstantiationTicket::operator!=(const SliceInstantiationTicket& rhs) const
    {
        return !((*this) == rhs);
    }

    bool SliceInstantiationTicket::IsValid() const
    {
        return m_requestId != 0;
    }

    AZStd::string SliceInstantiationTicket::ToString() const
    {
        return AZStd::string::format("{%s::%llu}", m_contextId.ToString<AZStd::string>().c_str(), m_requestId);
    }

    const EntityContextId& SliceInstantiationTicket::GetContextId() const
    {
        return m_contextId;
    }

    AZ::u64 SliceInstantiationTicket::GetRequestId() const
    {
        return m_requestId;
    }

}
