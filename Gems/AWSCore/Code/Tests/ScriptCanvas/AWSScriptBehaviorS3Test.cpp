/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
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

    ~AWSScriptBehaviorS3NotificationBusHandlerMock() override
    {
        AWSScriptBehaviorS3NotificationBus::Handler::BusDisconnect();
    }

    MOCK_METHOD1(OnHeadObjectSuccess, void(const AZStd::string&));
    MOCK_METHOD1(OnHeadObjectError, void(const AZStd::string&));
    MOCK_METHOD1(OnGetObjectSuccess, void(const AZStd::string&));
    MOCK_METHOD1(OnGetObjectError, void(const AZStd::string&));
};

class AWSScriptBehaviorS3Test
    : public AWSCoreFixture
{
public:
    void CreateReadOnlyTestFile(const AZStd::string& filePath)
    {
        AZ::IO::SystemFile file;
        if (!file.Open(
                filePath.c_str(),
                AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            AZ_Assert(false, "Failed to open test file at %s", filePath.c_str());
        }
        AZStd::string testContent = "It is a test file";
        if (file.Write(testContent.c_str(), testContent.size()) != testContent.size())
        {
            AZ_Assert(false, "Failed to write test file with content %s", testContent.c_str());
        }
        file.Close();
        AZ_Assert(AZ::IO::SystemFile::SetWritable(filePath.c_str(), false), "Failed to mark test file as read-only");
    }

    void RemoveReadOnlyTestFile(const AZStd::string& filePath)
    {
        if (!filePath.empty())
        {
            AZ_Assert(AZ::IO::SystemFile::SetWritable(filePath.c_str(), true), "Failed to mark test file as writeable");
            AZ_Assert(AZ::IO::SystemFile::Delete(filePath.c_str()), "Failed to delete test config file at %s", filePath.c_str());
        }
    }
};

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

TEST_F(AWSScriptBehaviorS3Test, GetObjectRaw_CallWithOutfileFailedToResolve_InvokeOnError)
{
    AWSScriptBehaviorS3NotificationBusHandlerMock s3HandlerMock;
    EXPECT_CALL(s3HandlerMock, OnGetObjectError(::testing::_)).Times(1);
    AWSScriptBehaviorS3::GetObjectRaw("dummyBucket", "dummyObject", "dummyRegion", "@dummy@/dummyOut.txt");
}

TEST_F(AWSScriptBehaviorS3Test, GetObjectRaw_CallWithOutfileNameIsDirectory_InvokeOnError)
{
    AWSScriptBehaviorS3NotificationBusHandlerMock s3HandlerMock;
    EXPECT_CALL(s3HandlerMock, OnGetObjectError(::testing::_)).Times(1);
    AWSScriptBehaviorS3::GetObjectRaw("dummyBucket", "dummyObject", "dummyRegion", AZ::Test::GetCurrentExecutablePath());
}

TEST_F(AWSScriptBehaviorS3Test, GetObjectRaw_CallWithOutfileDirectoryNoExist_InvokeOnError)
{
    AWSScriptBehaviorS3NotificationBusHandlerMock s3HandlerMock;
    EXPECT_CALL(s3HandlerMock, OnGetObjectError(::testing::_)).Times(1);
    AZStd::string dummyDirectory = AZStd::string::format("%s/dummyDirectory/dummyOut.txt", AZ::Test::GetCurrentExecutablePath().c_str());
    AWSScriptBehaviorS3::GetObjectRaw("dummyBucket", "dummyObject", "dummyRegion", dummyDirectory);
}

TEST_F(AWSScriptBehaviorS3Test, GetObjectRaw_CallWithOutfileIsReadOnly_InvokeOnError)
{
    AWSScriptBehaviorS3NotificationBusHandlerMock s3HandlerMock;
    EXPECT_CALL(s3HandlerMock, OnGetObjectError(::testing::_)).Times(1);
    AZStd::string randomTestFile = AZStd::string::format("%s/test%s.txt",
        AZ::Test::GetCurrentExecutablePath().c_str(), AZ::Uuid::CreateRandom().ToString<AZStd::string>(false, false).c_str());
    AzFramework::StringFunc::Path::Normalize(randomTestFile);
    CreateReadOnlyTestFile(randomTestFile);
    AWSScriptBehaviorS3::GetObjectRaw("dummyBucket", "dummyObject", "dummyRegion", randomTestFile);
    RemoveReadOnlyTestFile(randomTestFile);
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
