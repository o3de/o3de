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