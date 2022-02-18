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
}
