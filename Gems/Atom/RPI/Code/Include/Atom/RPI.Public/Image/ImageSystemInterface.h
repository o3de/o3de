/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RPI.Public/Configuration.h>
#include <AtomCore/Instance/Instance.h>

namespace AZ
{
    class Name;

    namespace RPI
    {
        class Image;
        class AttachmentImage;
        class AttachmentImagePool;
        class StreamingImagePool;

        enum class SystemImage : uint32_t
        {
            White,
            Black,
            Grey,
            Magenta,
            Count
        };

        namespace DefaultImageAssetPaths
        {
            static constexpr char DefaultFallback[] = "textures/defaults/defaultfallback.png.streamingimage";
            static constexpr char Processing[] = "textures/defaults/processing.png.streamingimage";
            static constexpr char ProcessingFailed[] = "textures/defaults/processingfailed.png.streamingimage";
            static constexpr char Missing[] = "textures/defaults/missing.png.streamingimage";
        }

        class ATOM_RPI_PUBLIC_API ImageSystemInterface
        {
        public:
            AZ_RTTI(ImageSystemInterface, "{C05FE364-927F-4669-ADDA-5562E20D5DC1}");

            ImageSystemInterface() = default;
            virtual ~ImageSystemInterface() = default;

            static ImageSystemInterface* Get();

            // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
            AZ_DISABLE_COPY_MOVE(ImageSystemInterface);

            //! Returns a system image generated at runtime.
            virtual const Data::Instance<Image>& GetSystemImage(SystemImage systemImage) const = 0;

            //! Returns a system attachment image generated at runtime for the given format. Supports
            //! color, depth, and depth/stencil attachment images
            virtual const Data::Instance<AttachmentImage>& GetSystemAttachmentImage(RHI::Format format) = 0;

            //! Returns the system streaming image pool. 
            virtual const Data::Instance<StreamingImagePool>& GetSystemStreamingPool() const = 0;

            //! O3DE_DEPRECATION_NOTICE(GHI-12058)
            //! @deprecated use GetSystemStreamingPool()
            virtual const Data::Instance<StreamingImagePool>& GetStreamingPool() const = 0;

            //! Returns the system attachment image pool. Use this if you do not need a custom pool for your allocation.
            virtual const Data::Instance<AttachmentImagePool>& GetSystemAttachmentPool() const = 0;
                        
            //! Register an attachment image by its unique name (attachment id)
            //! Return false if the image was failed to register.
            //!     It could be the image with same name was already registered. 
            //! Note: this function is only intended to be used by AttachmentImage class
            //!     Only attachment images created with an unique name will be registered
            virtual bool RegisterAttachmentImage(AttachmentImage* attachmentImage) = 0;
                        
            //! Unregister an attachment image (if it's was registered)
            virtual void UnregisterAttachmentImage(AttachmentImage* attachmentImage) = 0;

            //! Find an attachment image by its unique name (same as its attachment id) from registered attachment images.
            //! Note: only attachment image created with an unique name will be registered.
            virtual Data::Instance<AttachmentImage> FindRegisteredAttachmentImage(const Name& uniqueName) const = 0;

            virtual void Update() = 0;
        };
    }
}
