/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        virtual AZStd::string GetImageAssetPath() const = 0;
        virtual void SetImageAssetPath(const AZStd::string& assetPath) = 0;

        virtual float GetTilingX() const = 0;
        virtual void SetTilingX(float tilingX) = 0;

        virtual float GetTilingY() const = 0;
        virtual void SetTilingY(float tilingY) = 0;
    };

    using ImageGradientRequestBus = AZ::EBus<ImageGradientRequests>;
}
