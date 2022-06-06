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
    RemoteToolsMessage::~RemoteToolsMessage()
    {
        if (m_isBlobOwner)
        {
            azfree(const_cast<void*>(m_customBlob), AZ::OSAllocator);
        }
    }

    void RemoteToolsMessage::AddCustomBlob(const void* blob, size_t blobSize, bool ownBlob)
    {
        m_customBlob = blob;
        m_customBlobSize = static_cast<AZ::u32>(blobSize);
        m_isBlobOwner = ownBlob;
    }

    void RemoteToolsMessage::SetImmediateSelfDispatchEnabled(bool immediateSelfDispatchEnabled)
    {
        m_immediateSelfDispatch = immediateSelfDispatchEnabled;
    }

    bool RemoteToolsMessage::IsImmediateSelfDispatchEnabled() const
    {
        return m_immediateSelfDispatch;
    }

    const void* RemoteToolsMessage::GetCustomBlob() const
    {
        return m_customBlob;
    }
    size_t RemoteToolsMessage::GetCustomBlobSize() const
    {
        return m_customBlobSize;
    }

    AZ::u64 RemoteToolsMessage::GetId() const
    {
        return m_msgId;
    }

    AZ::u32 RemoteToolsMessage::GetSenderTargetId() const
    {
        return m_senderTargetId;
    }

    void RemoteToolsMessage::SetSenderTargetId(AZ::u32 senderTargetId)
    {
        m_senderTargetId = senderTargetId;
    }

    bool RemoteToolsEndpointInfo::IsSelf() const
    {
        return m_networkId == s_selfNetworkId;
    }

    bool RemoteToolsEndpointInfo::IsOnline() const
    {
        return m_isOnline;
    }

    bool RemoteToolsEndpointInfo::IsValid() const
    {
        return !m_displayName.empty() && m_networkId != 0 && m_persistentId != 0;
    }

    const char* RemoteToolsEndpointInfo::GetDisplayName() const
    {
        return m_displayName.c_str();
    }

    AZ::u32 RemoteToolsEndpointInfo::GetPersistentId() const
    {
        return m_persistentId;
    }

    AZ::u32 RemoteToolsEndpointInfo::GetNetworkId() const
    {
        return m_networkId;
    }

    bool RemoteToolsEndpointInfo::IsIdentityEqualTo(const RemoteToolsEndpointInfo& other) const
    {
        return m_persistentId == other.m_persistentId && m_networkId == other.m_networkId;
    }

    void RemoteToolsEndpointInfo::SetInfo(AZStd::string displayName, AZ::u32 persistentId, AZ::u32 networkId)
    {
        m_displayName = displayName;
        m_persistentId = persistentId;
        m_networkId = networkId;
    }
} // namespace AzFramework
