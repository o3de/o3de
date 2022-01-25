/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/condition_variable.h>

#include "HttpRequestManager.h"

class HttpTest
    : public UnitTest::ScopedAllocatorSetupFixture
{
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
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
