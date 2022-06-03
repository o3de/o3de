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
    ToolingMessage::~ToolingMessage()
    {
        if (m_isBlobOwner)
        {
            azfree(const_cast<void*>(m_customBlob), AZ::OSAllocator);
        }
    }

    void ToolingMessage::AddCustomBlob(const void* blob, size_t blobSize, bool ownBlob)
    {
        m_customBlob = blob;
        m_customBlobSize = static_cast<AZ::u32>(blobSize);
        m_isBlobOwner = ownBlob;
    }

    void ToolingMessage::SetImmediateSelfDispatchEnabled(bool immediateSelfDispatchEnabled)
    {
        m_immediateSelfDispatch = immediateSelfDispatchEnabled;
    }

    bool ToolingMessage::IsImmediateSelfDispatchEnabled() const
    {
        return m_immediateSelfDispatch;
    }

    const void* ToolingMessage::GetCustomBlob() const
    {
        return m_customBlob;
    }
    size_t ToolingMessage::GetCustomBlobSize() const
    {
        return m_customBlobSize;
    }

    AZ::u64 ToolingMessage::GetId() const
    {
        return m_msgId;
    }

    AZ::u32 ToolingMessage::GetSenderTargetId() const
    {
        return m_senderTargetId;
    }

    void ToolingMessage::SetSenderTargetId(AZ::u32 senderTargetId)
    {
        m_senderTargetId = senderTargetId;
    }

    bool ToolingEndpointInfo::IsSelf() const
    {
        return m_networkId == s_selfNetworkId;
    }

    bool ToolingEndpointInfo::IsOnline() const
    {
        return m_isOnline;
    }

    bool ToolingEndpointInfo::IsValid() const
    {
        return !m_displayName.empty() && m_networkId != 0 && m_persistentId != 0;
    }

    const char* ToolingEndpointInfo::GetDisplayName() const
    {
        return m_displayName.c_str();
    }

    AZ::u32 ToolingEndpointInfo::GetPersistentId() const
    {
        return m_persistentId;
    }

    AZ::u32 ToolingEndpointInfo::GetNetworkId() const
    {
        return m_networkId;
    }

    bool ToolingEndpointInfo::IsIdentityEqualTo(const ToolingEndpointInfo& other) const
    {
        return m_persistentId == other.m_persistentId && m_networkId == other.m_networkId;
    }

    void ToolingEndpointInfo::SetInfo(AZStd::string displayName, AZ::u32 persistentId, AZ::u32 networkId)
    {
        m_displayName = displayName;
        m_persistentId = persistentId;
        m_networkId = networkId;
    }
} // namespace AzFramework
