/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
