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