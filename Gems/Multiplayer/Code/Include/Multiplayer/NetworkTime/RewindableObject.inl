/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace Multiplayer
{
    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline RewindableObject<BASE_TYPE, REWIND_SIZE>::RewindableObject(const BASE_TYPE& value)
    {
        m_history.fill(value);
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline RewindableObject<BASE_TYPE, REWIND_SIZE>::RewindableObject(const BASE_TYPE& value, AzNetworking::ConnectionId owningConnectionId)
        : m_owningConnectionId(owningConnectionId)
        , m_history(value)
    {
        m_history.fill(value);
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline RewindableObject<BASE_TYPE, REWIND_SIZE>::RewindableObject(const RewindableObject<BASE_TYPE, REWIND_SIZE>& rhs)
        : m_owningConnectionId(rhs.m_owningConnectionId)
        , m_headTime(GetCurrentTimeForProperty())
        , m_headIndex(0)
    {
        m_history.fill(static_cast<BASE_TYPE>(rhs));
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline RewindableObject<BASE_TYPE, REWIND_SIZE> &RewindableObject<BASE_TYPE, REWIND_SIZE>::operator =(const BASE_TYPE& rhs)
    {
        SetValueForTime(rhs, GetCurrentTimeForProperty());
        return *this;
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline RewindableObject<BASE_TYPE, REWIND_SIZE> &RewindableObject<BASE_TYPE, REWIND_SIZE>::operator =(const RewindableObject<BASE_TYPE, REWIND_SIZE>& rhs)
    {
        INetworkTime* networkTime = Multiplayer::GetNetworkTime();
        SetValueForTime(rhs.GetValueForTime(networkTime->GetHostFrameId()), GetCurrentTimeForProperty());
        return *this;
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline void RewindableObject<BASE_TYPE, REWIND_SIZE>::SetOwningConnectionId(AzNetworking::ConnectionId owningConnectionId)
    {
        m_owningConnectionId = owningConnectionId;
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline RewindableObject<BASE_TYPE, REWIND_SIZE>::operator const BASE_TYPE& () const
    {
        return Get();
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline const BASE_TYPE& RewindableObject<BASE_TYPE, REWIND_SIZE>::Get() const
    {
        return GetValueForTime(GetCurrentTimeForProperty());
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline const BASE_TYPE& RewindableObject<BASE_TYPE, REWIND_SIZE>::GetPrevious() const
    {
        return GetValueForTime(GetPreviousTimeForProperty());
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline BASE_TYPE& RewindableObject<BASE_TYPE, REWIND_SIZE>::Modify()
    {
        const HostFrameId frameTime = GetCurrentTimeForProperty();
        if (frameTime < m_headTime)
        {
            AZ_Assert(false, "Trying to mutate a rewindable value in the past");
        }
        else if (m_headTime < frameTime)
        {
            SetValueForTime(GetValueForTime(frameTime), frameTime);
        }
        const BASE_TYPE& returnValue = GetValueForTime(frameTime);
        return const_cast<BASE_TYPE&>(returnValue);
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline bool RewindableObject<BASE_TYPE, REWIND_SIZE>::operator == (const BASE_TYPE& rhs) const
    {
        const BASE_TYPE lhs = GetValueForTime(GetCurrentTimeForProperty());
        return (lhs == rhs);
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline bool RewindableObject<BASE_TYPE, REWIND_SIZE>::operator != (const BASE_TYPE& rhs) const
    {
        const BASE_TYPE lhs = GetValueForTime(GetCurrentTimeForProperty());
        return (lhs != rhs);
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline bool RewindableObject<BASE_TYPE, REWIND_SIZE>::Serialize(AzNetworking::ISerializer& serializer)
    {
        const HostFrameId frameTime = GetCurrentTimeForProperty();
        BASE_TYPE value = GetValueForTime(frameTime);
        if (serializer.Serialize(value, "Element") && (serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject))
        {
            SetValueForTime(value, frameTime);
        }
        return serializer.IsValid();
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline HostFrameId RewindableObject<BASE_TYPE, REWIND_SIZE>::GetCurrentTimeForProperty() const
    {
        INetworkTime* networkTime = Multiplayer::GetNetworkTime();
        if (networkTime->IsTimeRewound() && (m_owningConnectionId == networkTime->GetRewindingConnectionId()))
        {
            return networkTime->GetUnalteredHostFrameId();
        }
        return networkTime->GetHostFrameId();
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline HostFrameId RewindableObject<BASE_TYPE, REWIND_SIZE>::GetPreviousTimeForProperty() const
    {
        INetworkTime* networkTime = Multiplayer::GetNetworkTime();
        if (networkTime->IsTimeRewound() && (m_owningConnectionId == networkTime->GetRewindingConnectionId()))
        {
            return networkTime->GetUnalteredHostFrameId();
        }
        return networkTime->GetHostFrameId() - HostFrameId(1);
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline void RewindableObject<BASE_TYPE, REWIND_SIZE>::SetValueForTime(const BASE_TYPE& value, HostFrameId frameTime)
    {
        if (frameTime < m_headTime)
        {
            // Don't try and set values older than our current head value
            return;
        }

        // Keeping a reference to copy to head so that delta bitset differences are only applied from the prev version
        const BASE_TYPE& prevHead = m_history[m_headIndex];

        if (static_cast<size_t>(frameTime - m_headTime) >= m_history.size())
        {
            // This update represents a large enough time delta that we'll just flush the whole buffer with the new value
            m_headTime = frameTime;
            m_headIndex = 0;
            for (uint32_t i = 0; i < m_history.size(); ++i)
            {
                m_history[i] = value;
            }
            return;
        }

        while (m_headTime < frameTime)
        {
            m_history[m_headIndex] = prevHead;
            m_headIndex = (m_headIndex + 1) % m_history.size();
            m_headTime++;
        }

        m_history[m_headIndex] = value;
        AZ_Assert(m_headTime == frameTime, "Invalid head value");
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline const BASE_TYPE &RewindableObject<BASE_TYPE, REWIND_SIZE>::GetValueForTime(HostFrameId frameTime) const
    {
        if (frameTime > m_headTime)
        {
            return m_history[m_headIndex];
        }
        const AZStd::size_t frameDelta = static_cast<AZStd::size_t>(m_headTime) - static_cast<AZStd::size_t>(frameTime);
        return m_history[GetOffsetIndex(frameDelta)];
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline AZStd::size_t RewindableObject<BASE_TYPE, REWIND_SIZE>::GetOffsetIndex(AZStd::size_t absoluteIndex) const
    {
        if (absoluteIndex >= m_history.size())
        {
            AZLOG(NET_Rewind, "Request for value which is too old");
            absoluteIndex = m_history.size() - 1;
        }
        return ((m_headIndex + m_history.size()) - absoluteIndex) % m_history.size();
    }
}
