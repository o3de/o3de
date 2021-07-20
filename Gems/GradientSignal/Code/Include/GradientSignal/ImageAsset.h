/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <Atom/ImageProcessing/PixelFormats.h>

namespace AZ
{
    class Vector3;
}

namespace GradientSignal
{
    constexpr const char* s_gradientImageExtension = "gradimage";

    /**
      * An asset that represents image data used for sampling a gradient signal image
      */      
    class ImageAsset 
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(ImageAsset, "{4DE8BBFB-EE42-4A6E-B3DB-17A719AC71F9}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(ImageAsset, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);
        static bool VersionConverter(AZ::SerializeContext& context,
            AZ::SerializeContext::DataElementNode& classElement);

        AZ::u32 m_imageWidth = 0;
        AZ::u32 m_imageHeight = 0;
        AZ::u8 m_bytesPerPixel = 0;
        ImageProcessingAtom::EPixelFormat m_imageFormat = ImageProcessingAtom::EPixelFormat::ePixelFormat_Unknown;
        AZStd::vector<AZ::u8> m_imageData;
    };

    class ImageAssetHandler
        : public AzFramework::GenericAssetHandler<ImageAsset>
    {
    public:
        AZ_CLASS_ALLOCATOR(ImageAssetHandler, AZ::SystemAllocator, 0);

        ImageAssetHandler()
            : AzFramework::GenericAssetHandler<ImageAsset>("Gradient Image", "Other", s_gradientImageExtension)
        {
        }
    };

    float GetValueFromImageAsset(const AZ::Data::Asset<ImageAsset>& imageAsset, const AZ::Vector3& uvw, float tilingX, float tilingY, float defaultValue);

} // namespace GradientSignal
