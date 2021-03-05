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
#include "HttpRequestor_precompiled.h"
#include <AzTest/AzTest.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/condition_variable.h>

#include "HttpRequestManager.h"

class Integ_HttpTest
    : public ::testing::Test
{
public:
    HttpRequestor::ManagerPtr m_httpRequestManager;

    // to wait for test to complete
    AZStd::mutex m_requestMutex;
    AZStd::condition_variable m_requestConditionVar;

    AZStd::string resultData;
    AZStd::atomic<Aws::Http::HttpResponseCode> resultCode;

    Integ_HttpTest()
    {
        m_httpRequestManager = AZStd::make_shared<HttpRequestor::Manager>();
        resultCode = Aws::Http::HttpResponseCode::REQUEST_NOT_MADE;
        resultData = "{}";

        AZStd::unique_lock<AZStd::mutex> lock(m_requestMutex);
        m_requestConditionVar.wait_for(lock, AZStd::chrono::milliseconds(10));
    }

    virtual ~Integ_HttpTest()
    {
        m_httpRequestManager.reset();
    }
};

TEST_F(Integ_HttpTest, HttpRequesterTest)
{
    m_httpRequestManager->AddTextRequest(HttpRequestor::TextParameters("https://httpbin.org/ip", Aws::Http::HttpMethod::HTTP_GET, [this](const AZStd::string & data, Aws::Http::HttpResponseCode code)
    {
        resultData = data;
        resultCode = code;
        m_requestConditionVar.notify_all();
    }));

    AZStd::unique_lock<AZStd::mutex> lock(m_requestMutex);
    m_requestConditionVar.wait_for(lock, AZStd::chrono::milliseconds(5000));

    EXPECT_NE(Aws::Http::HttpResponseCode::REQUEST_NOT_MADE, resultCode);
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
