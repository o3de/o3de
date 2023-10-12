/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI/ResourceView.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ::RHI
{
    class Scope;
    class FrameAttachment;
    class SingleDeviceSwapChain;
    
    struct ScopeAttachmentUsageAndAccess
    {
        /// How the attachment is used by the scope (render target, shader resource, depth stencil, etc.)
        ScopeAttachmentUsage m_usage = ScopeAttachmentUsage::Uninitialized;

        /// How the attachment is accessed by the scope (read, write or readwrite)
        ScopeAttachmentAccess m_access = ScopeAttachmentAccess::ReadWrite;
    };
    
        
    //! A ScopeAttachment is created when a FrameAttachment is "attached" to a specific scope. A single FrameAttachment
    //! exists for a given AttachemntId, but many ScopeAttachments can exist as "children" of a FrameAttachment. More
    //! precisely, ScopeAttachment forms a linked list, where the first node is the first "usage" on the first scope, and the last node
    //! the last usage on the last scope. FrameAttachment references the head and tail of this linked list.
    //!
    //! The FrameAttachment owns the Attachment instance (i.e. the actual resource). The ScopeAttachment owns a view into that
    //! resource. A scope is able to utilize the view during compilation and execution.         
    class ScopeAttachment
    {
        friend class FrameGraphAttachmentDatabase;
    public:
        AZ_RTTI(ScopeAttachment, "{6BB50E92-5A15-4C50-8717-F7B05AB98BD9}");

        ScopeAttachment(
            Scope& scope,
            FrameAttachment& attachment,
            ScopeAttachmentUsage usage,
            ScopeAttachmentAccess access);

        virtual ~ScopeAttachment() = default;
            
        //! Returns true if usage is compatible with how this scopeattachment will be used
        bool HasUsage(const ScopeAttachmentUsage usage) const;

        //! Returns true if access is compatible with how this scopeattachment will be accessed
        bool HasAccessAndUsage(const ScopeAttachmentUsage usage, const ScopeAttachmentAccess access) const;
            
        //! Returns a vector containing all usage/access data used by this scopeattachment.
        const AZStd::vector<ScopeAttachmentUsageAndAccess>& GetUsageAndAccess() const;
            
        //! Returns the resource view.
        const ResourceView* GetResourceView() const;

        //! Returns the parent scope that this attachment is bound to.
        const Scope& GetScope() const;
        Scope& GetScope();

        //! Returns the parent frame graph attachment.
        const FrameAttachment& GetFrameAttachment() const;
        FrameAttachment& GetFrameAttachment();

        //! Returns the previous binding in the linked list.
        const ScopeAttachment* GetPrevious() const;
        ScopeAttachment* GetPrevious();

        //! Returns the next binding in the linked list.
        const ScopeAttachment* GetNext() const;
        ScopeAttachment* GetNext();

        //! Returns the friendly usage and access type names of this scope attachment (used for logging).
        const char* GetTypeName(const RHI::ScopeAttachmentUsageAndAccess& usageAndAccess) const;
        AZStd::string GetUsageTypes() const;
        AZStd::string GetAccessTypes() const;

        //! Add how the attachment is used by the scope and how it is accessed by the scope.
        void AddUsageAndAccess(ScopeAttachmentUsage usage, ScopeAttachmentAccess access);
            
        //! Returns true if this is a SwapChainFrameAttachment
        bool IsSwapChainAttachment() const;            
            
    protected:

        //! Assigns the resource view to this scope attachment.
        void SetResourceView(ConstPtr<ResourceView> resourceView);

    private:
            
        //! Validates multiple usages/access for the same scopeattachment
        void ValidateMultipleScopeAttachmentUsages(const ScopeAttachmentUsage usage, const ScopeAttachmentAccess access);
            
        ScopeAttachment* m_prev = nullptr;
        ScopeAttachment* m_next = nullptr;

        /// The resource view declared for usage on this scope.
        ConstPtr<ResourceView> m_resourceView;

        /// The scope that the attachment is bound to.
        Scope* m_scope = nullptr;

        /// The attachment being bound.
        FrameAttachment* m_attachment = nullptr;

        AZStd::vector<ScopeAttachmentUsageAndAccess> m_usageAndAccess;
            
        bool m_isSwapChainAttachment = false;
    };

    using ScopeAttachmentPtrList = AZStd::vector<ScopeAttachment*>;
}
