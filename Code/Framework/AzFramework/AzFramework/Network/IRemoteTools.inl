/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzFramework
{
    inline RemoteToolsMessage::~RemoteToolsMessage()
    {
        if (m_isBlobOwner)
        {
            azfree(const_cast<AZStd::byte*>(m_customBlob.data()), AZ::OSAllocator, m_customBlob.size());
        }
    }

    inline void RemoteToolsMessage::AddCustomBlob(const void* blob, size_t blobSize, bool ownBlob)
    {
        m_customBlob = AZStd::span<AZStd::byte const>(reinterpret_cast<const AZStd::byte*>(blob), blobSize);
        m_isBlobOwner = ownBlob;
    }

    inline void RemoteToolsMessage::AddCustomBlob(AZStd::span<AZStd::byte const> blob, bool ownBlob)
    {
        m_customBlob = blob;
        m_isBlobOwner = ownBlob;
    }

    inline const AZStd::span<AZStd::byte const> RemoteToolsMessage::GetCustomBlob() const
    {
        return m_customBlob;
    }

    inline size_t RemoteToolsMessage::GetCustomBlobSize() const
    {
        return m_customBlob.size();
    }

    inline bool RemoteToolsMessage::GetIsBlobOwner() const
    {
        return m_isBlobOwner;
    }

    inline AZ::u64 RemoteToolsMessage::GetId() const
    {
        return m_msgId;
    }

    inline AZ::u32 RemoteToolsMessage::GetSenderTargetId() const
    {
        return m_senderTargetId;
    }

    inline void RemoteToolsMessage::SetSenderTargetId(AZ::u32 senderTargetId)
    {
        m_senderTargetId = senderTargetId;
    }

    inline void RemoteToolsMessage::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<RemoteToolsMessage>()->Field("MsgId", &RemoteToolsMessage::m_msgId);
        }
    }

    inline bool RemoteToolsEndpointInfo::IsSelf() const
    {
        return m_networkId == SelfNetworkId;
    }

    inline bool RemoteToolsEndpointInfo::IsOnline() const
    {
        return m_networkId != InvalidRemoteToolsConnectionId;
    }

    inline bool RemoteToolsEndpointInfo::IsValid() const
    {
        return !m_displayName.empty() && m_persistentId != 0;
    }

    inline const char* RemoteToolsEndpointInfo::GetDisplayName() const
    {
        return m_displayName.c_str();
    }

    inline AZ::u32 RemoteToolsEndpointInfo::GetPersistentId() const
    {
        return m_persistentId;
    }

    inline AZ::u32 RemoteToolsEndpointInfo::GetNetworkId() const
    {
        return m_networkId;
    }

    inline bool RemoteToolsEndpointInfo::IsIdentityEqualTo(const RemoteToolsEndpointInfo& other) const
    {
        return m_persistentId == other.m_persistentId && m_networkId == other.m_networkId;
    }

    inline void RemoteToolsEndpointInfo::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<RemoteToolsEndpointInfo>()
                ->Field("displayName", &RemoteToolsEndpointInfo::m_displayName)
                ->Field("persistentId", &RemoteToolsEndpointInfo::m_persistentId)
                ->Field("networkId", &RemoteToolsEndpointInfo::m_networkId);
        }
    }

    inline void RemoteToolsEndpointInfo::SetInfo(AZStd::string displayName, AZ::u32 persistentId, AZ::u32 networkId)
    {
        m_displayName = AZStd::move(displayName);
        m_persistentId = persistentId;
        m_networkId = networkId;
    }
} // namespace AzFramework
