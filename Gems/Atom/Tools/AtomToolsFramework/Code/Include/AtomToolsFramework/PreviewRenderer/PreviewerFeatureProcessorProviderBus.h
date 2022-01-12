/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>

namespace AtomToolsFramework
{
    //! PreviewerFeatureProcessorProviderRequests allows registering custom Feature Processors for preview image generation
    class PreviewerFeatureProcessorProviderRequests : public AZ::EBusTraits
    {
    public:
        //! Get a list of custom feature processors to register with preview image renderer
        virtual void GetRequiredFeatureProcessors(AZStd::unordered_set<AZStd::string>& featureProcessors) const = 0;
    };

    using PreviewerFeatureProcessorProviderBus = AZ::EBus<PreviewerFeatureProcessorProviderRequests>;
} // namespace AtomToolsFramework
