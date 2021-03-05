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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <ImageProcessing/PixelFormats.h>

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
        ImageProcessing::EPixelFormat m_imageFormat = ImageProcessing::EPixelFormat::ePixelFormat_Unknown;
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
