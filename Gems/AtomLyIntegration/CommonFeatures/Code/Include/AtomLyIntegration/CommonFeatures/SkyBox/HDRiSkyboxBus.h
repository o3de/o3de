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
#include <Atom/RPI.Public/Image/StreamingImage.h>

namespace AZ
{
    namespace Render
    {
        class HDRiSkyboxRequests
            : public ComponentBus
        {
        public:
            AZ_RTTI(AZ::Render::HDRiSkyboxRequests, "{86278300-438D-40B3-B87C-B87EE045373E}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            virtual ~HDRiSkyboxRequests() {}

            virtual Data::Asset<RPI::StreamingImageAsset> GetCubemapAsset() const = 0;
            virtual void SetCubemapAsset(const Data::Asset<RPI::StreamingImageAsset>& cubemapAsset) = 0;

            virtual void SetExposure(float exposure) = 0;
            virtual float GetExposure() const = 0; 
        };

        typedef AZ::EBus<HDRiSkyboxRequests> HDRiSkyboxRequestBus;
    }
}
