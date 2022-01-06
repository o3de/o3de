/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            //! Builders can implement this function to add job dependencies on other assets that may be used in the scene file conversion process.
            virtual void ReportJobDependencies(JobDependencyList& jobDependencyList, const char* platformIdentifier) { AZ_UNUSED(jobDependencyList); AZ_UNUSED(platformIdentifier); }
            
            //! Builders can implement this function to append to the job analysis fingerprint. This can be used to trigger rebuilds when global configuration changes.
            //! See also AssetBuilderDesc::m_analysisFingerprint.
            virtual void AddFingerprintInfo(AZStd::set<AZStd::string>& fingerprintInfo) { AZ_UNUSED(fingerprintInfo); }
        };
        using SceneBuilderDependencyBus = EBus<SceneBuilderDependencyRequests>;
    } // namespace SceneAPI
} // namespace AZ
