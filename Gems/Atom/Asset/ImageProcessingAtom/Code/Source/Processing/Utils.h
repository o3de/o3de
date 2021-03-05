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
    }
}
