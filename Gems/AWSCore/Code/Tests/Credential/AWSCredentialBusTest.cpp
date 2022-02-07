/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <aws/core/auth/AWSCredentialsProvider.h>

#include <Credential/AWSCredentialBus.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;

class TestCredentialHandlerOne
    : AWSCredentialRequestBus::Handler
{
public:
    TestCredentialHandlerOne()
    {
        m_handlerCounter = 0;
        m_credentialsProvider = std::make_shared<Aws::Auth::AnonymousAWSCredentialsProvider>();
    }

    void ActivateHandler()
    {
        AWSCredentialRequestBus::Handler::BusConnect();
    }

    void DeactivateHandler()
    {
        AWSCredentialRequestBus::Handler::BusDisconnect();
        m_credentialsProvider.reset();
    }

    int GetCredentialHandlerOrder() const override
    {
        return 1;
    }

    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetCredentialsProvider() override
    {
        ++m_handlerCounter;
        return m_credentialsProvider;
    }

    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> m_credentialsProvider;
    AZStd::atomic<int> m_handlerCounter;
};

class TestCredentialHandlerTwo
    : AWSCredentialRequestBus::Handler
{
public:
    TestCredentialHandlerTwo()
    {
        m_handlerCounter = 0;
        m_credentialsProvider = std::make_shared<Aws::Auth::AnonymousAWSCredentialsProvider>();
    }

    void ActivateHandler()
    {
        AWSCredentialRequestBus::Handler::BusConnect();
    }

    void DeactivateHandler()
    {
        AWSCredentialRequestBus::Handler::BusDisconnect();
        m_credentialsProvider.reset();
    }

    int GetCredentialHandlerOrder() const override
    {
        return 2;
    }

    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetCredentialsProvider() override
    {
        ++m_handlerCounter;
        return m_credentialsProvider;
    }

    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> m_credentialsProvider;
    AZStd::atomic<int> m_handlerCounter;
};

class AWSCredentialBusTest
    : public UnitTest::ScopedAllocatorSetupFixture
{
public:
    AWSCredentialBusTest()
    {
        m_handlerOne = AZStd::make_unique<TestCredentialHandlerOne>();
        m_handlerTwo = AZStd::make_unique<TestCredentialHandlerTwo>();
    }

    void SetUp() override
    {
        m_handlerOne->ActivateHandler();
        m_handlerTwo->ActivateHandler();
    }

    void TearDown() override
    {
        m_handlerOne->DeactivateHandler();
        m_handlerTwo->DeactivateHandler();
    }

    AZStd::unique_ptr<TestCredentialHandlerOne> m_handlerOne;
    AZStd::unique_ptr<TestCredentialHandlerTwo> m_handlerTwo;
};

TEST_F(AWSCredentialBusTest, GetCredentialsProvider_CallFromMultithread_GetExpectedCredentialsProviderAndNumberOfCalls)
{
    constexpr int testThreadNumber = 10;
    AZStd::atomic<int> actualEbusCalls = 0;
    AZStd::vector<AZStd::thread> testThreadPool;
    for (int index = 0; index < testThreadNumber; ++index)
    {
        testThreadPool.emplace_back(AZStd::thread([&]() {
            AWSCredentialResult result;
            AWSCredentialRequestBus::BroadcastResult(result, &AWSCredentialRequests::GetCredentialsProvider);
            ASSERT_TRUE(result.result);
            EXPECT_TRUE(result.result == m_handlerOne->m_credentialsProvider);
            actualEbusCalls++;
        }));
    }

    for (auto& testThread : testThreadPool)
    {
        testThread.join();
    }
    EXPECT_TRUE(m_handlerOne->m_handlerCounter == testThreadNumber);
    EXPECT_TRUE(m_handlerTwo->m_handlerCounter == testThreadNumber);
    EXPECT_TRUE(actualEbusCalls == testThreadNumber);
}
