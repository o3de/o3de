/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/FrameGraphAttachmentDatabase.h>
#include <Atom/RHI/SwapChainFrameAttachment.h>
#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/ImageFrameAttachment.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/SwapChain.h>
#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace RHI
    {
        size_t FrameGraphAttachmentDatabase::HashScopeAttachmentPair(const ScopeId& scopeId, const AttachmentId& attachmentId)
        {
            size_t seed = 0;
            AZStd::hash_combine(seed, scopeId.GetHash());
            AZStd::hash_combine(seed, attachmentId.GetHash());
            return seed;
        }

        ScopeId FrameGraphAttachmentDatabase::EmplaceResourcePoolUse(ResourcePool& pool, ScopeId scopeId)
        {
            ScopeId lastScope;
            auto found = m_resourcePoolLastScopeUse.find(&pool);        
            if (found != m_resourcePoolLastScopeUse.end())
            {
                lastScope = found->second;
                found->second = scopeId;
            }
            else
            {
                m_resourcePoolLastScopeUse.insert({ &pool, scopeId });
            }
            
            return lastScope;
        }

        bool FrameGraphAttachmentDatabase::ValidateAttachmentIsUnregistered(const AttachmentId& attachmentId) const
        {
            if (Validation::IsEnabled())
            {
                if (FindAttachment(attachmentId) != nullptr)
                {
                    AZ_Error("AttachmentDatabase", false, "Attachment with 'id' %s is already registered!", attachmentId.GetCStr());
                    return false;
                }
            }

            (void)attachmentId;
            return true;
        }

        ResultCode FrameGraphAttachmentDatabase::ImportSwapChain(
            const AttachmentId& attachmentId,
            Ptr<SwapChain> swapChain)
        {
            if (!ValidateAttachmentIsUnregistered(attachmentId))
            {
                return ResultCode::InvalidArgument;
            }

            SwapChainFrameAttachment* attachment = EmplaceFrameAttachment<SwapChainFrameAttachment>(attachmentId, AZStd::move(swapChain));
            m_imageAttachments.emplace_back(attachment);
            m_swapChainAttachments.emplace_back(attachment);
            return ResultCode::Success;
        }

        ResultCode FrameGraphAttachmentDatabase::ImportImage(
            const AttachmentId& attachmentId,
            Ptr<Image> image)
        {
            if (!ValidateAttachmentIsUnregistered(attachmentId))
            {
                return ResultCode::InvalidArgument;
            }

            ImageFrameAttachment* attachment = EmplaceFrameAttachment<ImageFrameAttachment>(attachmentId, AZStd::move(image));
            m_imageAttachments.emplace_back(attachment);
            m_importedImageAttachments.emplace_back(attachment);
            return ResultCode::Success;
        }

        ResultCode FrameGraphAttachmentDatabase::ImportBuffer(
            const AttachmentId& attachmentId,
            Ptr<Buffer> buffer)
        {
            if (!ValidateAttachmentIsUnregistered(attachmentId))
            {
                return ResultCode::InvalidArgument;
            }

            BufferFrameAttachment* attachment = EmplaceFrameAttachment<BufferFrameAttachment>(attachmentId, AZStd::move(buffer));
            m_bufferAttachments.emplace_back(attachment);
            m_importedBufferAttachments.emplace_back(attachment);
            return ResultCode::Success;
        }

        ResultCode FrameGraphAttachmentDatabase::CreateTransientImage(const TransientImageDescriptor& descriptor)
        {
            if (!ValidateAttachmentIsUnregistered(descriptor.m_attachmentId))
            {
                return ResultCode::InvalidArgument;
            }

            ImageFrameAttachment* attachment = EmplaceFrameAttachment<ImageFrameAttachment>(descriptor);
            m_imageAttachments.emplace_back(attachment);
            m_transientImageAttachments.emplace_back(attachment);
            return ResultCode::Success;
        }

        ResultCode FrameGraphAttachmentDatabase::CreateTransientBuffer(const TransientBufferDescriptor& descriptor)
        {
            if (!ValidateAttachmentIsUnregistered(descriptor.m_attachmentId))
            {
                return ResultCode::InvalidArgument;
            }

            BufferFrameAttachment* attachment = EmplaceFrameAttachment<BufferFrameAttachment>(descriptor);
            m_bufferAttachments.emplace_back(attachment);
            m_transientBufferAttachments.emplace_back(attachment);
            return ResultCode::Success;
        }

        void FrameGraphAttachmentDatabase::Clear()
        {
            m_scopeAttachmentLookup.clear();
            m_imageAttachments.clear();
            m_bufferAttachments.clear();
            m_swapChainAttachments.clear();
            m_importedImageAttachments.clear();
            m_importedBufferAttachments.clear();
            m_transientImageAttachments.clear();
            m_transientBufferAttachments.clear();
            m_attachmentLookup.clear();
            m_resourcePoolLastScopeUse.clear();

            for (ScopeAttachment* scopeAttachment : m_scopeAttachments)
            {
                delete scopeAttachment;
            }
            m_scopeAttachments.clear();

            for (FrameAttachment* attachment : m_attachments)
            {
                delete attachment;
            }
            m_attachments.clear();
        }

        ImageDescriptor FrameGraphAttachmentDatabase::GetImageDescriptor(const AttachmentId& attachmentId) const
        {
            const ImageFrameAttachment* imageAttachment = FindAttachment<ImageFrameAttachment>(attachmentId);
            if (imageAttachment)
            {
                return imageAttachment->GetImageDescriptor();
            }
            return ImageDescriptor{};
        }

        BufferDescriptor FrameGraphAttachmentDatabase::GetBufferDescriptor(const AttachmentId& attachmentId) const
        {
            const BufferFrameAttachment* bufferAttachment = FindAttachment<BufferFrameAttachment>(attachmentId);
            if (bufferAttachment)
            {
                return bufferAttachment->GetBufferDescriptor();
            }
            return BufferDescriptor{};
        }

        bool FrameGraphAttachmentDatabase::IsAttachmentValid(const AttachmentId& attachmentId) const
        {
            return FindAttachment(attachmentId) != nullptr;
        }

        const FrameAttachment* FrameGraphAttachmentDatabase::FindAttachment(const AttachmentId& attachmentId) const
        {
            auto findIt = m_attachmentLookup.find(attachmentId);
            if (findIt != m_attachmentLookup.end())
            {
                return findIt->second;
            }
            return nullptr;
        }

        FrameAttachment* FrameGraphAttachmentDatabase::FindAttachment(const AttachmentId& attachmentId)
        {
            return const_cast<FrameAttachment*>(const_cast<const FrameGraphAttachmentDatabase*>(this)->FindAttachment(attachmentId));
        }

        const ScopeAttachmentPtrList* FrameGraphAttachmentDatabase::FindScopeAttachmentList(const ScopeId& scopeId, const AttachmentId& attachmentId) const
        {
            auto findIt = m_scopeAttachmentLookup.find(HashScopeAttachmentPair(scopeId, attachmentId));
            if (findIt != m_scopeAttachmentLookup.end())
            {
                return &findIt->second;
            }
            return nullptr;
        }

        const ScopeAttachment* FrameGraphAttachmentDatabase::FindScopeAttachment(const ScopeId& scopeId, const AttachmentId& attachmentId, size_t index) const
        {
            const ScopeAttachmentPtrList* scopeAttachmentList = FindScopeAttachmentList(scopeId, attachmentId);
            if (!scopeAttachmentList)
            {
                return nullptr;
            }

            if (index >= scopeAttachmentList->size())
            {
                AZ_Error("AttachmentDatabase", false,
                    "Attempting to access scope attachment [%d], but list only has [%d] elements. ScopeId: [%s]. AttachmentId: [%s]",
                    index,
                    scopeAttachmentList->size(),
                    scopeId.GetCStr(),
                    attachmentId.GetCStr());

                return nullptr;
            }

            return (*scopeAttachmentList)[index];
        }

        const AZStd::vector<ImageFrameAttachment*>& FrameGraphAttachmentDatabase::GetImageAttachments() const
        {
            return m_imageAttachments;
        }

        const AZStd::vector<BufferFrameAttachment*>& FrameGraphAttachmentDatabase::GetBufferAttachments() const
        {
            return m_bufferAttachments;
        }

        const AZStd::vector<SwapChainFrameAttachment*>& FrameGraphAttachmentDatabase::GetSwapChainAttachments() const
        {
            return m_swapChainAttachments;
        }

        const AZStd::vector<ImageFrameAttachment*>& FrameGraphAttachmentDatabase::GetImportedImageAttachments() const
        {
            return m_importedImageAttachments;
        }

        const AZStd::vector<BufferFrameAttachment*>& FrameGraphAttachmentDatabase::GetImportedBufferAttachments() const
        {
            return m_importedBufferAttachments;
        }

        const AZStd::vector<ImageFrameAttachment*>& FrameGraphAttachmentDatabase::GetTransientImageAttachments() const
        {
            return m_transientImageAttachments;
        }

        const AZStd::vector<BufferFrameAttachment*>& FrameGraphAttachmentDatabase::GetTransientBufferAttachments() const
        {
            return m_transientBufferAttachments;
        }

        const AZStd::vector<FrameAttachment*>& FrameGraphAttachmentDatabase::GetAttachments() const
        {
            return m_attachments;
        }

        const AZStd::vector<ScopeAttachment*>& FrameGraphAttachmentDatabase::GetScopeAttachments() const
        {
            return m_scopeAttachments;
        }
    }
}
