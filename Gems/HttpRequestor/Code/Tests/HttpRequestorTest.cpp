/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/parallel/condition_variable.h>
#include "HttpRequestManager.h"
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Debug/IEventLogger.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Debug/LocalFileEventLogger.h>

class HttpTest
    : public UnitTest::ScopedAllocatorSetupFixture
    , public AZ::Debug::TraceMessageBus::Handler
{
public:

    bool OnPrintf(const char*, const char* message) override
    {
        m_gatheredMessages.emplace_back(message);
        AZ::StringFunc::TrimWhiteSpace(m_gatheredMessages.back(), true, true);
        return true;
    }

    virtual void SetUp() override
    {
        BusConnect();

        // AWSNativeSDKInit requires these if logging occurs, normally would be set
        // up by the ComponentApplication
        if (!AZ::Interface<AZ::IConsole>::Get())
        {
            m_console = AZStd::make_unique<AZ::Console>();
            m_console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());
            AZ::Interface<AZ::IConsole>::Register(m_console.get());
        }
        if (!AZ::Interface<AZ::Debug::IEventLogger>::Get())
        {
            m_eventLogger.reset(new AZ::Debug::LocalFileEventLogger);
        }
    }

    void TearDown() override
    {
        if (m_console)
        {
            AZ::Interface<AZ::IConsole>::Unregister(m_console.get());
            m_console.reset();
        }
        if (m_eventLogger)
        {
            m_eventLogger.reset();
        }
        m_gatheredMessages = {};

        BusDisconnect();
    }

    AZStd::vector<AZStd::string> m_gatheredMessages;

private:

    AZStd::unique_ptr<AZ::Console> m_console;
    AZStd::unique_ptr<AZ::Debug::LocalFileEventLogger> m_eventLogger; 
};

TEST_F(HttpTest, HttpRequesterTest)
{
    HttpRequestor::Manager httpRequestManager;

    // to wait for test to complete
    AZStd::mutex requestMutex;
    AZStd::condition_variable requestConditionVar;

    AZStd::string resultData = {};
    AZStd::atomic<Aws::Http::HttpResponseCode> resultCode = Aws::Http::HttpResponseCode::REQUEST_NOT_MADE;

    {
        AZStd::unique_lock<AZStd::mutex> lock(requestMutex);
        requestConditionVar.wait_for(lock, AZStd::chrono::milliseconds(10));
    }

    httpRequestManager.AddTextRequest(HttpRequestor::TextParameters(
        "https://httpbin.org/ip", Aws::Http::HttpMethod::HTTP_GET,
        [&resultData, &resultCode, &requestConditionVar](const AZStd::string& data, Aws::Http::HttpResponseCode code)
        {
            resultData = data;
            resultCode = code;
            requestConditionVar.notify_all();
        }));

    {
        AZStd::unique_lock<AZStd::mutex> lock(requestMutex);
        requestConditionVar.wait_for(lock, AZStd::chrono::milliseconds(5000));
    }

    EXPECT_NE(Aws::Http::HttpResponseCode::REQUEST_NOT_MADE, resultCode);

    for (const auto& message : m_gatheredMessages)
    {
        // Expect to see zero lines about reading credentials from the Amazon EC2 instance metadata service (IMDS). 
        EXPECT_FALSE(message.contains("EC2MetadataClient - Http request to retrieve credentials failed")) << "Found unexpected line in output: " << message.c_str();
    }
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
