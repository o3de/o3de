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

#include <AzCore/Component/Component.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Asset/AssetCatalog.h>

namespace TestAssetBuilder
{
    struct TestDependentAsset
        : public AZ::Data::AssetData
    {
        AZ_CLASS_ALLOCATOR(TestDependentAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(TestDependentAsset, "{B91BCEFE-1725-47E8-A762-C09F09425904}", AZ::Data::AssetData);

        TestDependentAsset() = default;

    };

    class TestDependentAssetCatalog
        : public AZ::Data::AssetCatalog
    {
    public:
        AZ_CLASS_ALLOCATOR(TestDependentAssetCatalog, AZ::SystemAllocator, 0);

        TestDependentAssetCatalog() = default;

        AZ::Data::AssetStreamInfo GetStreamInfoForLoad(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& type) override;
    };

    //! TestAssetBuilderComponent handles the lifecycle of the builder.
    class TestAssetBuilderComponent
        : public AZ::Component,
          public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_COMPONENT(TestAssetBuilderComponent, "{55C3848D-A489-4428-9BA9-4A40AC7B9952}");

        TestAssetBuilderComponent();
        ~TestAssetBuilderComponent() override;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        //////////////////////////////////////////////////////////////////////////
        //!AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override; // if you get this you must fail all existing jobs and return.
        //////////////////////////////////////////////////////////////////////////

    private:

        bool m_isShuttingDown = false;
        AZStd::unique_ptr<TestDependentAssetCatalog> m_dependentCatalog;
    };
} // namespace TestAssetBuilder
