/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Tests/GradientSignalTestMocks.h>

namespace UnitTest
{
    AZ::Data::Asset<GradientSignal::ImageAsset> ImageAssetMockAssetHandler::CreateImageAsset(AZ::u32 width, AZ::u32 height, AZ::s32 seed)
    {
        GradientSignal::ImageAsset* imageData =
            aznew GradientSignal::ImageAsset(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), AZ::Data::AssetData::AssetStatus::Ready);
        imageData->m_imageWidth = width;
        imageData->m_imageHeight = height;
        imageData->m_bytesPerPixel = 1;
        imageData->m_imageFormat = ImageProcessingAtom::EPixelFormat::ePixelFormat_R8;
        imageData->m_imageData.reserve(width * height);

        size_t value = 0;
        AZStd::hash_combine(value, seed);

        for (AZ::u32 x = 0; x < width; ++x)
        {
            for (AZ::u32 y = 0; y < height; ++y)
            {
                AZStd::hash_combine(value, x);
                AZStd::hash_combine(value, y);
                imageData->m_imageData.push_back(static_cast<AZ::u8>(value));
            }
        }

        return AZ::Data::Asset<GradientSignal::ImageAsset>(imageData, AZ::Data::AssetLoadBehavior::Default);
    }

    AZ::Data::Asset<GradientSignal::ImageAsset> ImageAssetMockAssetHandler::CreateSpecificPixelImageAsset(
        AZ::u32 width, AZ::u32 height, AZ::u32 pixelX, AZ::u32 pixelY)
    {
        GradientSignal::ImageAsset* imageData =
            aznew GradientSignal::ImageAsset(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), AZ::Data::AssetData::AssetStatus::Ready);
        imageData->m_imageWidth = width;
        imageData->m_imageHeight = height;
        imageData->m_bytesPerPixel = 1;
        imageData->m_imageFormat = ImageProcessingAtom::EPixelFormat::ePixelFormat_R8;
        imageData->m_imageData.reserve(width * height);

        const AZ::u8 pixelValue = 255;

        // Image data should be stored inverted on the y axis relative to our engine, so loop backwards through y.
        for (int y = static_cast<int>(height) - 1; y >= 0; --y)
        {
            for (AZ::u32 x = 0; x < width; ++x)
            {
                if ((x == static_cast<int>(pixelX)) && (y == static_cast<int>(pixelY)))
                {
                    imageData->m_imageData.push_back(pixelValue);
                }
                else
                {
                    imageData->m_imageData.push_back(0);
                }
            }
        }

        return AZ::Data::Asset<GradientSignal::ImageAsset>(imageData, AZ::Data::AssetLoadBehavior::Default);
    }
}

