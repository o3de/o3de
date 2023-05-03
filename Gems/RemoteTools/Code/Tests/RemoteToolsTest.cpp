/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Console/LoggerSystemComponent.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Time/TimeSystem.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzFramework/Network/IRemoteTools.h>
#include <AzNetworking/Framework/NetworkingSystemComponent.h>

#include <RemoteToolsSystemComponent.h>

namespace UnitTest
{
    using namespace RemoteTools;

    static constexpr AZ::Crc32 TestToolsKey("TestRemoteTools"); 

    class RemoteToolsTests : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            AZ::NameDictionary::Create();

            m_timeSystem = AZStd::make_unique<AZ::TimeSystem>();
            m_networkingSystemComponent = AZStd::make_unique<AzNetworking::NetworkingSystemComponent>();
            m_remoteToolsSystemComponent = AZStd::make_unique<RemoteToolsSystemComponent>();
            m_remoteTools = m_remoteToolsSystemComponent.get();
        }

        void TearDown() override
        {
            m_remoteTools = nullptr;
            m_remoteToolsSystemComponent.reset();
            m_networkingSystemComponent.reset();
            m_timeSystem.reset();

            AZ::NameDictionary::Destroy();
            LeakDetectionFixture::SetUp();
        }

        AZStd::unique_ptr<AzNetworking::NetworkingSystemComponent> m_networkingSystemComponent;
        AZStd::unique_ptr<RemoteToolsSystemComponent> m_remoteToolsSystemComponent;
        AZStd::unique_ptr<AZ::TimeSystem> m_timeSystem;
        AzFramework::IRemoteTools* m_remoteTools = nullptr;
    };

    TEST_F(RemoteToolsTests, TEST_RemoteToolsEmptyRegistry)
    {
        EXPECT_NE(m_remoteTools, nullptr);
        EXPECT_EQ(m_remoteTools->GetReceivedMessages(TestToolsKey), nullptr);
        AzFramework::RemoteToolsEndpointContainer endpointContainer;

        m_remoteTools->EnumTargetInfos(TestToolsKey, endpointContainer);
        EXPECT_TRUE(endpointContainer.empty());

        AzFramework::RemoteToolsEndpointInfo endpointInfo;
        endpointInfo = m_remoteTools->GetDesiredEndpoint(TestToolsKey);
        EXPECT_FALSE(endpointInfo.IsValid());
        endpointInfo = m_remoteTools->GetEndpointInfo(TestToolsKey, 0);
        EXPECT_FALSE(endpointInfo.IsValid());
        EXPECT_FALSE(m_remoteTools->IsEndpointOnline(TestToolsKey, 0));
    }

    TEST_F(RemoteToolsTests, TEST_RemoteToolsHost)
    {
        EXPECT_NE(m_remoteTools, nullptr);
        m_remoteTools->RegisterToolingServiceHost(TestToolsKey, AZ::Name("Test"), 6999);
        EXPECT_EQ(m_remoteTools->GetReceivedMessages(TestToolsKey), nullptr);
        AzFramework::RemoteToolsEndpointContainer endpointContainer;

        m_remoteTools->EnumTargetInfos(TestToolsKey, endpointContainer);
        EXPECT_EQ(endpointContainer.size(), 1);

        m_remoteTools->SetDesiredEndpoint(TestToolsKey, TestToolsKey);
        AzFramework::RemoteToolsEndpointInfo endpointInfo;
        endpointInfo = m_remoteTools->GetDesiredEndpoint(TestToolsKey);
        EXPECT_TRUE(endpointInfo.IsValid());
        EXPECT_TRUE(endpointInfo.IsSelf());
        EXPECT_FALSE(m_remoteTools->IsEndpointOnline(TestToolsKey, TestToolsKey));

        {
            AzFramework::RemoteToolsMessage msg;
            msg.SetSenderTargetId(TestToolsKey);
            m_remoteTools->SendRemoteToolsMessage(endpointInfo, msg);
        }
        const AzFramework::ReceivedRemoteToolsMessages* receiveMsgs = m_remoteTools->GetReceivedMessages(TestToolsKey);
        EXPECT_NE(receiveMsgs, nullptr);
        EXPECT_EQ(receiveMsgs->size(), 1);
        m_remoteTools->ClearReceivedMessages(TestToolsKey);
    }

}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
