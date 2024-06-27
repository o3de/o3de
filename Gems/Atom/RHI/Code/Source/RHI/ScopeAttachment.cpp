/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/ScopeAttachment.h>
#include <Atom/RHI/SwapChainFrameAttachment.h>
#include <Atom/RHI/FrameAttachment.h>

namespace AZ::RHI
{
    ScopeAttachment::ScopeAttachment(
        Scope& scope,
        FrameAttachment& attachment,
        ScopeAttachmentUsage usage,
        ScopeAttachmentAccess access,
        ScopeAttachmentStage stage)
        : m_scope{&scope}
        , m_attachment{&attachment}
        , m_usage(usage)
        , m_access(access)
        , m_stage(stage)
    {
        m_isSwapChainAttachment = azrtti_cast<const RHI::SwapChainFrameAttachment*>(m_attachment) != nullptr;
    }

    const Scope& ScopeAttachment::GetScope() const
    {
        return *m_scope;
    }

    Scope& ScopeAttachment::GetScope()
    {
        return *m_scope;
    }

    const FrameAttachment& ScopeAttachment::GetFrameAttachment() const
    {
        return *m_attachment;
    }

    FrameAttachment& ScopeAttachment::GetFrameAttachment()
    {
        return *m_attachment;
    }   
  
    ScopeAttachmentUsage ScopeAttachment::GetUsage() const
    {
        return m_usage;
    }

    ScopeAttachmentAccess ScopeAttachment::GetAccess() const
    {
        return m_access;
    }

    ScopeAttachmentStage ScopeAttachment::GetStage() const
    {
        return m_stage;
    }

    const ScopeAttachment* ScopeAttachment::GetPrevious() const
    {
        return m_prev;
    }

    ScopeAttachment* ScopeAttachment::GetPrevious()
    {
        return m_prev;
    }

    const ScopeAttachment* ScopeAttachment::GetNext() const
    {
        return m_next;
    }

    ScopeAttachment* ScopeAttachment::GetNext()
    {
        return m_next;
    }
    
    const char* ScopeAttachment::GetTypeName() const
    {
        return ToString(m_usage, m_access);
    }
    
    const ResourceView* ScopeAttachment::GetResourceView() const
    {
        return m_resourceView.get();
    }

    void ScopeAttachment::SetResourceView(ConstPtr<ResourceView> resourceView)
    {
        m_resourceView = AZStd::move(resourceView);
    }

    bool ScopeAttachment::IsSwapChainAttachment() const
    {
        return m_isSwapChainAttachment;
    }
}
