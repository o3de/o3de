/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/EBus/EBus.h>

namespace AZ
{
    namespace SceneAPI
    {
        using JobDependencyList = AZStd::vector<AssetBuilderSDK::JobDependency>;
        
        /**
        * Ebus interface for registering SceneAPI Exporter dependencies, primarily used with asset builder components
        */
        class SceneBuilderDependencyRequests
            : public AZ::EBusTraits
        {
        public:
            virtual void ReportJobDependencies(JobDependencyList& jobDependencyList, const char* platformIdentifier) = 0;
        };
        using SceneBuilderDependencyBus = EBus<SceneBuilderDependencyRequests>;
    } // namespace SceneAPI
} // namespace AZ
