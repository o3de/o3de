/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

#include <GradientSignal/Util.h>

namespace GradientSignal
{
    class ImageGradientRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Get the path to the processed image asset.
        //! @return The image asset path
        virtual AZStd::string GetImageAssetPath() const = 0;

        //! Get the path to the source image asset.
        //! @return The source image asset path
        virtual AZStd::string GetImageAssetSourcePath() const = 0;

        //! Set the image asset for the component by providing a path to the processed image asset.
        //! @param assetPath The path to the processed image asset
        virtual void SetImageAssetPath(const AZStd::string& assetPath) = 0;

        //! Set the image asset for the component by providing a path to the source image asset.
        //! @param assetPath The path to the source image asset
        virtual void SetImageAssetSourcePath(const AZStd::string& assetPath) = 0;

        //! Get the image height
        //! @return The image height in pixels
        virtual uint32_t GetImageHeight() const = 0;

        //! Get the image width
        //! @return The image width in pixels
        virtual uint32_t GetImageWidth() const = 0;

        //! Get the image pixels per meter
        //! Using the Image Gradient's mapping of an image to world space, this returns how many pixels per meter exist in each direction.
        //! This information is useful for image painting/modifications to know the correct spacing for modifying the image pixel-by-pixel.
        //! @return The number of pixels per meter in the X and Y directions.
        virtual AZ::Vector2 GetImagePixelsPerMeter() const = 0;

        //! Gets the tiling setting for the X direction
        //! A value of 1 is a 1:1 mapping of the image to the gradient bounds.
        //! @return Tiling factor for X
        virtual float GetTilingX() const = 0;

        //! Sets the tiling setting for the X direction
        //! @param tilingX Tiling factor for X
        virtual void SetTilingX(float tilingX) = 0;

        //! Gets the tiling setting for the Y direction
        //! A value of 1 is a 1:1 mapping of the image to the gradient bounds.
        //! @return Tiling factor for Y
        virtual float GetTilingY() const = 0;

        //! Sets the tiling setting for the Y direction
        //! @param tilingY Tiling factor for Y
        virtual void SetTilingY(float tilingY) = 0;
    };

    using ImageGradientRequestBus = AZ::EBus<ImageGradientRequests>;
}
