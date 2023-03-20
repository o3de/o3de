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
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>

namespace AZ
{
    class ReflectContext;
}

namespace TestAssetBuilder
{
    class TestAsset : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(TestAsset, "{3BDE90FA-B163-4FB9-BC67-22AC2ABD8C28}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(TestAsset, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        TestAsset() = default;
        virtual ~TestAsset() = default;

        AZStd::vector<AZ::Data::Asset<TestAsset>> m_referencedAssets;
    };

    //! This builder is intended for automated tests which need an asset that can reference other assets.
    //! It will take .auto_test_input files containing a single path to a source file and output .auto_test_asset files with an asset
    //! reference to the assumed product of the referenced asset.  References should be to other .auto_test_input files.
    class TestDependencyBuilderComponent
        : public AZ::Component,
          public AssetBuilderSDK::AssetBuilderCommandBus::MultiHandler
    {
    public:
        AZ_COMPONENT(TestDependencyBuilderComponent, "{E6DEE36F-8F75-41CB-9FEC-7E3231A97C1F}");

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
    };
} // namespace TestAssetBuilder
