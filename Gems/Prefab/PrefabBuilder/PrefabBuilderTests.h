/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PrefabBuilderComponent.h>
#include <AzTest/AzTest.h>
#include <Application/ToolsApplication.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Asset/AssetSerializer.h>

namespace UnitTest
{
    struct VersionChangingData final : AZ::Data::AssetData
    {
        AZ_RTTI(VersionChangingData, "{E3A37E19-AE61-4C2F-809E-03B4D83261E8}", AZ::Data::AssetData);

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<VersionChangingData>()->Version(m_version);
            }
        }

        inline static int m_version = 0;
    };

    struct TestAsset final : AZ::Data::AssetData
    {
        AZ_RTTI(TestAsset, "{8E736462-5424-4720-A2D9-F71DFC5905E3}", AZ::Data::AssetData);

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestAsset>();
            }
        }
    };

    struct TestAssetHandler final : AZ::Data::AssetHandler
    {
    public:
        AZ::Data::AssetPtr CreateAsset(
            [[maybe_unused]] const AZ::Data::AssetId& id, [[maybe_unused]] const AZ::Data::AssetType& type) override
        {
            return aznew TestAsset();
        }

        void DestroyAsset(AZ::Data::AssetPtr ptr) override
        {
            delete ptr;
        }

        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override
        {
            assetTypes.push_back(azrtti_typeid<TestAsset>());
        }

        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            [[maybe_unused]] const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            [[maybe_unused]] AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
        {
            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
        }
    };

    struct TestComponent final : AZ::Component
    {
        AZ_COMPONENT(TestComponent, "{E3982C6A-0B01-4B04-A3E2-D95729D4B9C6}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestComponent, AZ::Component>()
                    ->Field("Asset", &TestComponent::m_asset)
                    ->Field("Version", &TestComponent::m_version);
            }
        }

        void Activate() override
        {
        }
        void Deactivate() override
        {
        }
        
        AZ::Data::Asset<TestAsset> m_asset{};
        VersionChangingData m_version;
    };

    struct SelfContainedMemoryStream : AZ::IO::ByteContainerStream<AZStd::vector<char>>
    {
        SelfContainedMemoryStream() : ByteContainerStream<AZStd::vector<char>>{&m_bufferData}
        {
        }

        AZStd::vector<char> m_bufferData;
    };

    struct TestPrefabBuilderComponent final : AZ::Prefab::PrefabBuilderComponent
    {
    protected:
        AZStd::unique_ptr<AZ::IO::GenericStream> GetOutputStream(const AZ::IO::Path& path) const override;
    };

    struct PrefabBuilderTests : ::testing::Test
    {
    protected:
        void SetUp() override;
        void TearDown() override;

        AzToolsFramework::ToolsApplication m_app;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_testComponentDescriptor{};
        TestAssetHandler m_assetHandler;
    };
}
