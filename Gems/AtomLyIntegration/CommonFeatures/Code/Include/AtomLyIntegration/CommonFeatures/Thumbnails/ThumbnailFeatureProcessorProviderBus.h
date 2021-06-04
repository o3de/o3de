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

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            //! ThumbnailFeatureProcessorProviderRequests allows registering custom Feature Processors for thumbnail generation
            //! Duplicates will be ignored
            //! You can check minimal feature processors that are already registered in CommonThumbnailRenderer.cpp
            class ThumbnailFeatureProcessorProviderRequests
                : public AZ::EBusTraits
            {
            public:
                //! Get a list of custom feature processors to register with thumbnail renderer
                virtual const AZStd::vector<AZStd::string>& GetCustomFeatureProcessors() const = 0;
            };

            using ThumbnailFeatureProcessorProviderBus = AZ::EBus<ThumbnailFeatureProcessorProviderRequests>;
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
