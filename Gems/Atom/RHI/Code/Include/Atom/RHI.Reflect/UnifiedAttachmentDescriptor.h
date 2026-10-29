/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <AzCore/std/containers/variant.h>

namespace AZ::RHI
{
    struct ImageAttachment
    {
        ImageDescriptor m_image;
        ImageViewDescriptor m_imageView;
    };
    struct BufferAttachment
    {
        BufferDescriptor m_buffer;
        BufferViewDescriptor m_bufferView;
    };
    struct ResolveAttachment
    {
    };
    struct UninitializedAttachment
    {
    };
    //! A unified attachment descriptor used to store either an image or a buffer descriptor.
    //! Used primarily to simplify pass attachment logic while supporting both attachment types.
    struct ATOM_RHI_REFLECT_API UnifiedAttachmentDescriptor
        : AZStd::variant<ImageAttachment, BufferAttachment, ResolveAttachment, UninitializedAttachment>
    {
        using Base = AZStd::variant<ImageAttachment, BufferAttachment, ResolveAttachment, UninitializedAttachment>;
        UnifiedAttachmentDescriptor();
        UnifiedAttachmentDescriptor(const BufferDescriptor& bufferDescriptor);
        UnifiedAttachmentDescriptor(const ImageDescriptor& imageDescriptor);
        UnifiedAttachmentDescriptor(const BufferDescriptor& bufferDescriptor, const BufferViewDescriptor& bufferViewDescriptor);
        UnifiedAttachmentDescriptor(const ImageDescriptor& imageDescriptor, const ImageViewDescriptor& imageViewDescriptor);

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

        template<class T>
        constexpr decltype(auto) get() &
        {
            return AZStd::get<T>(*this);
        }

        template<class T>
        constexpr decltype(auto) get() const&
        {
            return AZStd::get<T>(*this);
        }

        template<class T>
        constexpr decltype(auto) get() &&
        {
            return AZStd::get<T>(AZStd::move(*this));
        }

        template<class T>
        constexpr decltype(auto) get() const&&
        {
            return AZStd::get<T>(AZStd::move(*this));
        }

        constexpr auto type() const -> AttachmentType
        {
            return std::array{
                AttachmentType::Image, AttachmentType::Buffer, AttachmentType::Resolve, AttachmentType::Uninitialized
            }[index()];
        }
    };
} // namespace AZ::RHI
