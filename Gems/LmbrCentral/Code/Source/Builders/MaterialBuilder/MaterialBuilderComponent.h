/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
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

#include <AzCore/Component/Component.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace MaterialBuilder
{
    //! Material builder is responsible for building material files
    class MaterialBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        MaterialBuilderWorker();
        ~MaterialBuilderWorker();

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        //!AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override;

        //! Returns the UUID for this builder
        static AZ::Uuid GetUUID();

        bool GetResolvedTexturePathsFromMaterial(const AZStd::string& path, AZStd::vector<AZStd::string>& resolvedPaths);
        bool PopulateProductDependencyList(AZStd::vector<AZStd::string>& resolvedPaths, AssetBuilderSDK::ProductPathDependencySet& dependencies);

    private:
        bool GatherProductDependencies(const AZStd::string& path, AssetBuilderSDK::ProductPathDependencySet& dependencies);

        bool m_isShuttingDown = false;
    };

    class BuilderPluginComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(BuilderPluginComponent, "{4D1A4B0C-54CE-4397-B8AE-ADD08898C2CD}")
        static void Reflect(AZ::ReflectContext* context);

        BuilderPluginComponent();

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        virtual void Init(); // create objects, allocate memory and initialize yourself without reaching out to the outside world
        virtual void Activate(); // reach out to the outside world and connect up to what you need to, register things, etc.
        virtual void Deactivate(); // unregister things, disconnect from the outside world
        //////////////////////////////////////////////////////////////////////////

        virtual ~BuilderPluginComponent(); // free memory an uninitialize yourself.

    private:
        MaterialBuilderWorker m_materialBuilder;
    };
}
