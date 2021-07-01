/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <Builders/CopyDependencyBuilder/CopyDependencyBuilderWorker.h>

namespace CopyDependencyBuilder
{
    //! The EmfxWorkspace builder is a copy job builder that examines asset files for asset references,
    //! to output product dependencies.
    class EmfxWorkspaceBuilderWorker
        : public CopyDependencyBuilderWorker
    {
    public:

    public:
        AZ_RTTI(EmfxWorkspaceBuilderWorker, "{E1863C77-040F-41C0-8A84-87A1BFD088DC}", CopyDependencyBuilderWorker);

        EmfxWorkspaceBuilderWorker();

        void RegisterBuilderWorker() override;
        void UnregisterBuilderWorker() override;
        bool ParseProductDependencies(
            const AssetBuilderSDK::ProcessJobRequest& request,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& pathDependencies) override;
    };
}
