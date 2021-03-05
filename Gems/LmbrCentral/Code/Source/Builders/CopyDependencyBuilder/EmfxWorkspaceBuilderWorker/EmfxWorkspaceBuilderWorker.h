/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
