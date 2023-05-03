/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(TestDependentAsset, AZ::SystemAllocator);
        AZ_RTTI(TestDependentAsset, "{B91BCEFE-1725-47E8-A762-C09F09425904}", AZ::Data::AssetData);

        TestDependentAsset() = default;

    };

    class TestDependentAssetCatalog
        : public AZ::Data::AssetCatalog
    {
    public:
        AZ_CLASS_ALLOCATOR(TestDependentAssetCatalog, AZ::SystemAllocator);

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
