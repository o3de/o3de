/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
