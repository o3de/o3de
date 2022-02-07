/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/ImageDescriptor.h>

#include <AtomCore/Instance/Instance.h>

namespace AZ
{
    namespace RPI
    {
        class Image;
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

        class ImageSystemInterface
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

            //! Returns the system streaming image pool. Use this if you do not need a custom pool for your allocation.
            //! This pool's budget is usually small and it's preferred to use it only for streaming images which data were
            //! generated during runtime instead of from streaming image assets.
            virtual const Data::Instance<StreamingImagePool>& GetSystemStreamingPool() const = 0;

            //! Returns streaming image pool which is dedicated for streaming images created from streaming image assets.
            virtual const Data::Instance<StreamingImagePool>& GetStreamingPool() const = 0;

            //! Returns the system attachment image pool. Use this if you do not need a custom pool for your allocation.
            virtual const Data::Instance<AttachmentImagePool>& GetSystemAttachmentPool() const = 0;
            

            virtual void Update() = 0;
        };
    }
}
