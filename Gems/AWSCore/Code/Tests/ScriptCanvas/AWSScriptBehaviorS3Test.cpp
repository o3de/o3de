/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
 * a third party where indicated.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzTest/AzTest.h>

#include <ScriptCanvas/AWSScriptBehaviorS3.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;

class AWSScriptBehaviorS3NotificationBusHandlerMock
    : public AWSScriptBehaviorS3NotificationBusHandler
{
public:
    AWSScriptBehaviorS3NotificationBusHandlerMock()
    {
        AWSScriptBehaviorS3NotificationBus::Handler::BusConnect();
    }

    ~AWSScriptBehaviorS3NotificationBusHandlerMock()
    {
        AWSScriptBehaviorS3NotificationBus::Handler::BusDisconnect();
    }

    MOCK_METHOD1(OnHeadObjectSuccess, void(const AZStd::string&));
    MOCK_METHOD1(OnHeadObjectError, void(const AZStd::string&));
    MOCK_METHOD1(OnGetObjectSuccess, void(const AZStd::string&));
    MOCK_METHOD1(OnGetObjectError, void(const AZStd::string&));
};

using AWSScriptBehaviorS3Test = UnitTest::ScopedAllocatorSetupFixture;

TEST_F(AWSScriptBehaviorS3Test, HeadObjectRaw_CallWithEmptyBucketName_InvokeOnError)
{
    AWSScriptBehaviorS3NotificationBusHandlerMock s3HandlerMock;
    EXPECT_CALL(s3HandlerMock, OnHeadObjectError(::testing::_)).Times(1);
    AWSScriptBehaviorS3::HeadObjectRaw("", "dummyObject", "dummyRegion");
}

TEST_F(AWSScriptBehaviorS3Test, HeadObjectRaw_CallWithEmptyObjectKeyName_InvokeOnError)
{
    AWSScriptBehaviorS3NotificationBusHandlerMock s3HandlerMock;
    EXPECT_CALL(s3HandlerMock, OnHeadObjectError(::testing::_)).Times(1);
    AWSScriptBehaviorS3::HeadObjectRaw("dummyBucket", "", "dummyRegion");
}

TEST_F(AWSScriptBehaviorS3Test, HeadObjectRaw_CallWithEmptyRegionName_InvokeOnError)
{
    AWSScriptBehaviorS3NotificationBusHandlerMock s3HandlerMock;
    EXPECT_CALL(s3HandlerMock, OnHeadObjectError(::testing::_)).Times(1);
    AWSScriptBehaviorS3::HeadObjectRaw("dummyBucket", "dummyObject", "");
}

TEST_F(AWSScriptBehaviorS3Test, HeadObject_NoBucketNameInResourceMappingFound_InvokeOnError)
{
    AWSScriptBehaviorS3NotificationBusHandlerMock s3HandlerMock;
    EXPECT_CALL(s3HandlerMock, OnHeadObjectError(::testing::_)).Times(1);
    AWSScriptBehaviorS3::HeadObject("dummyBucket", "dummyObject");
}

TEST_F(AWSScriptBehaviorS3Test, GetObjectRaw_CallWithEmptyBucketName_InvokeOnError)
{
    AWSScriptBehaviorS3NotificationBusHandlerMock s3HandlerMock;
    EXPECT_CALL(s3HandlerMock, OnGetObjectError(::testing::_)).Times(1);
    AWSScriptBehaviorS3::GetObjectRaw("", "dummyObject", "dummyRegion", "dummyOut");
}

TEST_F(AWSScriptBehaviorS3Test, GetObjectRaw_CallWithEmptyObjectKeyName_InvokeOnError)
{
    AWSScriptBehaviorS3NotificationBusHandlerMock s3HandlerMock;
    EXPECT_CALL(s3HandlerMock, OnGetObjectError(::testing::_)).Times(1);
    AWSScriptBehaviorS3::GetObjectRaw("dummyBucket", "", "dummyRegion", "dummyOut");
}

TEST_F(AWSScriptBehaviorS3Test, GetObjectRaw_CallWithEmptyRegionName_InvokeOnError)
{
    AWSScriptBehaviorS3NotificationBusHandlerMock s3HandlerMock;
    EXPECT_CALL(s3HandlerMock, OnGetObjectError(::testing::_)).Times(1);
    AWSScriptBehaviorS3::GetObjectRaw("dummyBucket", "dummyObject", "", "dummyOut");
}

TEST_F(AWSScriptBehaviorS3Test, GetObjectRaw_CallWithEmptyOutfileName_InvokeOnError)
{
    AWSScriptBehaviorS3NotificationBusHandlerMock s3HandlerMock;
    EXPECT_CALL(s3HandlerMock, OnGetObjectError(::testing::_)).Times(1);
    AWSScriptBehaviorS3::GetObjectRaw("dummyBucket", "dummyObject", "dummyRegion", "");
}

TEST_F(AWSScriptBehaviorS3Test, GetObject_NoBucketNameInResourceMappingFound_InvokeOnError)
{
    AWSScriptBehaviorS3NotificationBusHandlerMock s3HandlerMock;
    EXPECT_CALL(s3HandlerMock, OnGetObjectError(::testing::_)).Times(1);
    AWSScriptBehaviorS3::GetObject("dummyBucket", "dummyObject", "dummyOut");
}

TEST_F(AWSScriptBehaviorS3Test, OnSuccessOnError_Call_GetExpectedNumOfInvoke)
{
    AWSScriptBehaviorS3NotificationBusHandlerMock s3HandlerMock;
    EXPECT_CALL(s3HandlerMock, OnGetObjectSuccess(::testing::_)).Times(1);
    EXPECT_CALL(s3HandlerMock, OnGetObjectError(::testing::_)).Times(1);
    EXPECT_CALL(s3HandlerMock, OnHeadObjectSuccess(::testing::_)).Times(1);
    EXPECT_CALL(s3HandlerMock, OnHeadObjectError(::testing::_)).Times(1);
    AWSScriptBehaviorS3NotificationBus::Broadcast(&AWSScriptBehaviorS3NotificationBus::Events::OnGetObjectSuccess, "dummy success message");
    AWSScriptBehaviorS3NotificationBus::Broadcast(&AWSScriptBehaviorS3NotificationBus::Events::OnGetObjectError, "dummy success message");
    AWSScriptBehaviorS3NotificationBus::Broadcast(&AWSScriptBehaviorS3NotificationBus::Events::OnHeadObjectSuccess, "dummy success message");
    AWSScriptBehaviorS3NotificationBus::Broadcast(&AWSScriptBehaviorS3NotificationBus::Events::OnHeadObjectError, "dummy success message");
}
