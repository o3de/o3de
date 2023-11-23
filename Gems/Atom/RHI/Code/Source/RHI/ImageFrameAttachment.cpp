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
        Ptr<SingleDeviceImage> image)
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

    const ImageScopeAttachment* ImageFrameAttachment::GetFirstScopeAttachment() const
    {
        return static_cast<const ImageScopeAttachment*>(FrameAttachment::GetFirstScopeAttachment());
    }

    ImageScopeAttachment* ImageFrameAttachment::GetFirstScopeAttachment()
    {
        return static_cast<ImageScopeAttachment*>(FrameAttachment::GetFirstScopeAttachment());
    }

    const ImageScopeAttachment* ImageFrameAttachment::GetLastScopeAttachment() const
    {
        return static_cast<const ImageScopeAttachment*>(FrameAttachment::GetLastScopeAttachment());
    }

    ImageScopeAttachment* ImageFrameAttachment::GetLastScopeAttachment()
    {
        return static_cast<ImageScopeAttachment*>(FrameAttachment::GetLastScopeAttachment());
    }

    const ImageDescriptor& ImageFrameAttachment::GetImageDescriptor() const
    {
        return m_imageDescriptor;
    }

    const SingleDeviceImage* ImageFrameAttachment::GetImage() const
    {
        return static_cast<const SingleDeviceImage*>(GetResource());
    }

    SingleDeviceImage* ImageFrameAttachment::GetImage()
    {
        return static_cast<SingleDeviceImage*>(GetResource());
    }

    ClearValue ImageFrameAttachment::GetOptimizedClearValue() const
    {
        if (m_hasClearValueOverride)
        {
            return m_clearValueOverride;
        }

        const ScopeAttachment* bindingNode = GetFirstScopeAttachment();
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
