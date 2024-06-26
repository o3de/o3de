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
#include <Atom/RHI/Resource.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ::RHI
{
    class Scope;
    class FrameAttachment;
    struct ScopeAttachmentDescriptor;
    
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
            ScopeAttachmentAccess access,
            ScopeAttachmentStage stage);

        virtual ~ScopeAttachment() = default;

        //! Returns the usage
        ScopeAttachmentUsage GetUsage() const;

        //! Returns the access
        ScopeAttachmentAccess GetAccess() const;

        //! Returns the pipeline stage
        ScopeAttachmentStage GetStage() const;

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
        const char* GetTypeName() const;

        //! Returns true if this is a SwapChainFrameAttachment
        bool IsSwapChainAttachment() const;

        //! Return the ScopeAttachmentDescriptor used by this attachment
        virtual const ScopeAttachmentDescriptor& GetScopeAttachmentDescriptor() const = 0;
            
    protected:

        //! Assigns the resource view to this scope attachment.
        void SetResourceView(ConstPtr<ResourceView> resourceView);

    private:
            
        ScopeAttachment* m_prev = nullptr;
        ScopeAttachment* m_next = nullptr;

        /// The resource view declared for usage on this scope.
        ConstPtr<ResourceView> m_resourceView;

        /// The scope that the attachment is bound to.
        Scope* m_scope = nullptr;

        /// The attachment being bound.
        FrameAttachment* m_attachment = nullptr;

        /// How the attachment is used by the scope (render target, shader resource, depth stencil, etc.)
        ScopeAttachmentUsage m_usage = ScopeAttachmentUsage::Uninitialized;

        /// How the attachment is accessed by the scope (read, write or readwrite)
        ScopeAttachmentAccess m_access = ScopeAttachmentAccess::ReadWrite;

        // In which pipeline stages is being used (VertexShader, FragmentShader, LateFragmentTest, etc.)
        ScopeAttachmentStage m_stage = ScopeAttachmentStage::Any;
            
        bool m_isSwapChainAttachment = false;
    };

    using ScopeAttachmentPtrList = AZStd::vector<ScopeAttachment*>;
}
