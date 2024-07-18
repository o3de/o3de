/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/ImageFrameAttachment.h>
#include <Atom/RHI/ImageScopeAttachment.h>

namespace AZ::RHI
{
    ImageFrameAttachment::ImageFrameAttachment(
        const AttachmentId& attachmentId,
        Ptr<Image> image)
        : FrameAttachment(
            attachmentId,
            HardwareQueueClassMask::All,
            AttachmentLifetimeType::Imported)
        , m_imageDescriptor{image->GetDescriptor()}
    {
        SetResource(AZStd::move(image));
    }

    ImageFrameAttachment::ImageFrameAttachment(const TransientImageDescriptor& descriptor)
        : FrameAttachment(
            descriptor.m_attachmentId,
            descriptor.m_supportedQueueMask,
            AttachmentLifetimeType::Transient)
        , m_imageDescriptor{descriptor.m_imageDescriptor}
    {
        if (descriptor.m_optimizedClearValue)
        {
            m_clearValueOverride = *descriptor.m_optimizedClearValue;
            m_hasClearValueOverride = true;
        }
    }

    const ImageScopeAttachment* ImageFrameAttachment::GetFirstScopeAttachment(int deviceIndex) const
    {
        return static_cast<const ImageScopeAttachment*>(FrameAttachment::GetFirstScopeAttachment(deviceIndex));
    }

    ImageScopeAttachment* ImageFrameAttachment::GetFirstScopeAttachment(int deviceIndex)
    {
        return static_cast<ImageScopeAttachment*>(FrameAttachment::GetFirstScopeAttachment(deviceIndex));
    }

    const ImageScopeAttachment* ImageFrameAttachment::GetLastScopeAttachment(int deviceIndex) const
    {
        return static_cast<const ImageScopeAttachment*>(FrameAttachment::GetLastScopeAttachment(deviceIndex));
    }

    ImageScopeAttachment* ImageFrameAttachment::GetLastScopeAttachment(int deviceIndex)
    {
        return static_cast<ImageScopeAttachment*>(FrameAttachment::GetLastScopeAttachment(deviceIndex));
    }

    const ImageDescriptor& ImageFrameAttachment::GetImageDescriptor() const
    {
        return m_imageDescriptor;
    }

    const Image* ImageFrameAttachment::GetImage() const
    {
        return static_cast<const Image*>(GetResource());
    }

    Image* ImageFrameAttachment::GetImage()
    {
        return static_cast<Image*>(GetResource());
    }

    ClearValue ImageFrameAttachment::GetOptimizedClearValue(int deviceIndex) const
    {
        if (m_hasClearValueOverride)
        {
            return m_clearValueOverride;
        }

        const ScopeAttachment* bindingNode = GetFirstScopeAttachment(deviceIndex);
        while (bindingNode)
        {
            const ImageScopeAttachmentDescriptor& binding = static_cast<const ImageScopeAttachment&>(*bindingNode).GetDescriptor();

            if (binding.m_loadStoreAction.m_loadAction == AttachmentLoadAction::Clear ||
                binding.m_loadStoreAction.m_loadActionStencil == AttachmentLoadAction::Clear)
            {
                return binding.m_loadStoreAction.m_clearValue;
            }

            bindingNode = bindingNode->GetNext();
        }
        return ClearValue{};
    }
}
