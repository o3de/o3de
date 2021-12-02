/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzTest/AzTest.h>

#include <ScriptCanvas/AWSScriptBehaviorLambda.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;

class AWSScriptBehaviorLambdaNotificationBusHandlerMock : public AWSScriptBehaviorLambdaNotificationBusHandler
{
public:
    AWSScriptBehaviorLambdaNotificationBusHandlerMock()
    {
        AWSScriptBehaviorLambdaNotificationBus::Handler::BusConnect();
    }

    ~AWSScriptBehaviorLambdaNotificationBusHandlerMock() override
    {
        AWSScriptBehaviorLambdaNotificationBus::Handler::BusDisconnect();
    }

    MOCK_METHOD1(OnInvokeSuccess, void(const AZStd::string&));
    MOCK_METHOD1(OnInvokeError, void(const AZStd::string&));
};

using AWSScriptBehaviorLambdaTest = UnitTest::ScopedAllocatorSetupFixture;

TEST_F(AWSScriptBehaviorLambdaTest, InvokeRaw_CallWithEmptyFunctionName_InvokeOnError)
{
    AWSScriptBehaviorLambdaNotificationBusHandlerMock lambdaHandlerMock;
    EXPECT_CALL(lambdaHandlerMock, OnInvokeError(::testing::_)).Times(1);
    AWSScriptBehaviorLambda::InvokeRaw("", "dummyPayload", "dummyRegion");
}

TEST_F(AWSScriptBehaviorLambdaTest, InvokeRaw_CallWithEmptyRegionName_InvokeOnError)
{
    AWSScriptBehaviorLambdaNotificationBusHandlerMock lambdaHandlerMock;
    EXPECT_CALL(lambdaHandlerMock, OnInvokeError(::testing::_)).Times(1);
    AWSScriptBehaviorLambda::InvokeRaw("dummyFunction", "dummyPayload", "");
}

TEST_F(AWSScriptBehaviorLambdaTest, Invoke_NoFunctionNameInResourceMappingFound_InvokeOnError)
{
    AWSScriptBehaviorLambdaNotificationBusHandlerMock lambdaHandlerMock;
    EXPECT_CALL(lambdaHandlerMock, OnInvokeError(::testing::_)).Times(1);
    AWSScriptBehaviorLambda::Invoke("dummyFunction", "dummyPayload");
}

TEST_F(AWSScriptBehaviorLambdaTest, OnSuccessOnError_Call_GetExpectedNumOfInvoke)
{
    AWSScriptBehaviorLambdaNotificationBusHandlerMock lambdaHandlerMock;
    EXPECT_CALL(lambdaHandlerMock, OnInvokeSuccess(::testing::_)).Times(1);
    EXPECT_CALL(lambdaHandlerMock, OnInvokeError(::testing::_)).Times(1);
    AWSScriptBehaviorLambdaNotificationBus::Broadcast(&AWSScriptBehaviorLambdaNotificationBus::Events::OnInvokeSuccess, "dummy success message");
    AWSScriptBehaviorLambdaNotificationBus::Broadcast(&AWSScriptBehaviorLambdaNotificationBus::Events::OnInvokeError, "dummy error message");
}
