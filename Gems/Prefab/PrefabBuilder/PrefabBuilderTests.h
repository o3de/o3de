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

#include <PrefabBuilderComponent.h>
#include <AzTest/AzTest.h>
#include <Application/ToolsApplication.h>
#include <AzCore/Component/ComponentApplication.h>

namespace UnitTest
{
    struct VersionChangingData : AZ::Data::AssetData
    {
        AZ_TYPE_INFO(VersionChangingData, "{E3A37E19-AE61-4C2F-809E-03B4D83261E8}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<VersionChangingData>()->Version(m_version);
            }
        }

        inline static int m_version = 0;
    };

    struct TestAsset : AZ::Data::AssetData
    {
        AZ_TYPE_INFO(TestAsset, "{8E736462-5424-4720-A2D9-F71DFC5905E3}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestAsset>();
            }
        }
    };

    struct TestComponent : AZ::Component
    {
        AZ_COMPONENT(TestComponent, "{E3982C6A-0B01-4B04-A3E2-D95729D4B9C6}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestComponent>()
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

    struct TestPrefabBuilderComponent : AZ::Prefab::PrefabBuilderComponent
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
    };
}
