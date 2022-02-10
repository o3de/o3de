/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/ImageAsset.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Debug/Profiler.h>

#include <Atom/ImageProcessing/PixelFormats.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <GradientSignal/Util.h>
#include <numeric>

namespace GradientSignal
{
    void ImageAsset::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ImageAsset, AZ::Data::AssetData>()
                ->Version(1, &ImageAsset::VersionConverter)
                ->Field("Width", &ImageAsset::m_imageWidth)
                ->Field("Height", &ImageAsset::m_imageHeight)
                ->Field("BytesPerPixel", &ImageAsset::m_bytesPerPixel)
                ->Field("Format", &ImageAsset::m_imageFormat)
                ->Field("Data", &ImageAsset::m_imageData)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<ImageAsset>(
                    "Image Asset", "")
                    ->DataElement(0, &ImageAsset::m_imageWidth, "Width", "Image width.")
                    ->DataElement(0, &ImageAsset::m_imageHeight, "Height", "Image height.")
                    ->DataElement(0, &ImageAsset::m_bytesPerPixel, "BytesPerPixel", "Image bytes per pixel.")
                    ->DataElement(0, &ImageAsset::m_imageFormat, "Format", "Image format.")
                    ->DataElement(0, &ImageAsset::m_imageData, "Data", "Image color data.")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                    ;
            }
        }
    }

    bool ImageAsset::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() < 1)
        {
            int formatIndex = classElement.FindElement(AZ_CRC("Format", 0xdeba72df));

            if (formatIndex < 0)
            {
                return false;
            }

            AZ::SerializeContext::DataElementNode& format = classElement.GetSubElement(formatIndex);
            if (format.Convert<ImageProcessingAtom::EPixelFormat>(context))
            {
                format.SetData<ImageProcessingAtom::EPixelFormat>(context, ImageProcessingAtom::EPixelFormat::ePixelFormat_R8);
            }

            int bppIndex = classElement.AddElement<AZ::u8>(context, "BytesPerPixel");

            AZ::SerializeContext::DataElementNode& bpp = classElement.GetSubElement(bppIndex);
            bpp.SetData<AZ::u8>(context, 1);
        }

        return true;
    }

    float GetValueFromImageAsset(AZStd::span<const uint8_t> imageData, const AZ::RHI::ImageDescriptor& imageDescriptor, const AZ::Vector3& uvw, float tilingX, float tilingY, float defaultValue)
    {
        if (!imageData.empty())
        {
            auto width = imageDescriptor.m_size.m_width;
            auto height = imageDescriptor.m_size.m_height;

            if (width > 0 && height > 0)
            {
                // When "rasterizing" from uvs, a range of 0-1 has slightly different meanings depending on the sampler state.
                // For repeating states (Unbounded/None, Repeat), a uv value of 1 should wrap around back to our 0th pixel.
                // For clamping states (Clamp to Zero, Clamp to Edge), a uv value of 1 should point to the last pixel.

                // We assume here that the code handling sampler states has handled this for us in the clamping cases
                // by reducing our uv by a small delta value such that anything that wants the last pixel has a value
                // just slightly less than 1.

                // Keeping that in mind, we scale our uv from 0-1 to 0-image size inclusive.  So a 4-pixel image will scale
                // uv values of 0-1 to 0-4, not 0-3 as you might expect.  This is because we want the following range mappings:
                // [0 - 1/4)   = pixel 0
                // [1/4 - 1/2) = pixel 1
                // [1/2 - 3/4) = pixel 2
                // [3/4 - 1)   = pixel 3
                // [1 - 1 1/4) = pixel 0
                // ...

                // Also, based on our tiling settings, we extend the size of our image virtually by a factor of tilingX and tilingY.  
                // A 16x16 pixel image and tilingX = tilingY = 1  maps the uv range of 0-1 to 0-16 pixels.  
                // A 16x16 pixel image and tilingX = tilingY = 1.5 maps the uv range of 0-1 to 0-24 pixels.

                const AZ::Vector3 tiledDimensions((width  * tilingX),
                    (height * tilingY),
                    0.0f);

                // Convert from uv space back to pixel space
                AZ::Vector3 pixelLookup = (uvw * tiledDimensions);

                // UVs outside the 0-1 range are treated as infinitely tiling, so that we behave the same as the 
                // other gradient generators.  As mentioned above, if clamping is desired, we expect it to be applied
                // outside of this function.
                auto x = aznumeric_cast<AZ::u32>(pixelLookup.GetX()) % width;
                auto y = aznumeric_cast<AZ::u32>(pixelLookup.GetY()) % height;

                // Flip the y because images are stored in reverse of our world axes
                y = (height - 1) - y;

                return AZ::RPI::GetImageDataPixelValue<float>(imageData, imageDescriptor, x, y);
            }
        }

        return defaultValue;
    }
}
