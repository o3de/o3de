/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Atom/ImageProcessing/ImageObject.h>

namespace AssetBuilderSDK
{
    struct JobProduct;
}

namespace ImageProcessingAtom
{
    class ImageProcessingRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        typedef AZStd::recursive_mutex MutexType;
        //////////////////////////////////////////////////////////////////////////

        // Loads an image from a source file path
        virtual IImageObjectPtr LoadImage(const AZStd::string& filePath) = 0;

        // Loads an image from a source file path and converts it to a format suitable for previewing in tools
        virtual IImageObjectPtr LoadImagePreview(const AZStd::string& filePath) = 0;
    };
    using ImageProcessingRequestBus = AZ::EBus<ImageProcessingRequests>;

    class ImageBuilderRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        /////////////////////////////////////////////////////////////////////////

        //! Create an image object
        virtual IImageObjectPtr CreateImage(
            AZ::u32 width,
            AZ::u32 height,
            AZ::u32 maxMipCount,
            EPixelFormat pixelFormat) = 0;

        //! Convert an image and save its products to the specified folder
        virtual AZStd::vector<AssetBuilderSDK::JobProduct> ConvertImageObject(
            IImageObjectPtr imageObject,
            const AZStd::string& presetName,
            const AZStd::string& platformName,
            const AZStd::string& outputDir,
            const AZ::Data::AssetId& sourceAssetId,
            const AZStd::string& sourceAssetName) = 0;

        //! Return whether the specified platform is supported by the image builder
        virtual bool DoesSupportPlatform(const AZStd::string& platformId) = 0;

        //! Return whether the specified preset requires an image to be square and a power of 2
        virtual bool IsPresetFormatSquarePow2(const AZStd::string& presetName, const AZStd::string& platformName) = 0;
    };

    using ImageBuilderRequestBus = AZ::EBus<ImageBuilderRequests>;
} // namespace ImageProcessingAtom
