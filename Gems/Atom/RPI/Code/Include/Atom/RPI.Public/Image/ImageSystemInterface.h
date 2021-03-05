/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
