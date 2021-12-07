/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/ImageProcessing/ImageObject.h>
#include <AzCore/Asset/AssetCommon.h>

namespace ImageProcessingAtom
{
    namespace Utils
    {
        EPixelFormat RHIFormatToPixelFormat(AZ::RHI::Format rhiFormat, bool& isSRGB);

        AZ::RHI::Format PixelFormatToRHIFormat(EPixelFormat pixelFormat, bool isSrgb);

        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> LoadImageAsset(const AZ::Data::AssetId& assetId);

        IImageObjectPtr LoadImageFromImageAsset(const AZ::Data::AssetId& assetId);

        IImageObjectPtr LoadImageFromImageAsset(const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& asset);

        bool SaveImageToDdsFile(IImageObjectPtr image, AZStd::string_view filePath);

        bool NeedAlphaChannel(EAlphaContent alphaContent);
    }
}
