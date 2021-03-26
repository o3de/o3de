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

#include <AzFramework/API/ApplicationAPI.h>
#include <source/utils/utils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Utils/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>


namespace AssetBundler
{
    class MockUtilsTest
        : public UnitTest::ScopedAllocatorSetupFixture
        , public AzFramework::ApplicationRequests::Bus::Handler
    {
    public:
        void SetUp() override
        {
            ScopedAllocatorSetupFixture::SetUp();
            AzFramework::ApplicationRequests::Bus::Handler::BusConnect();
            m_localFileIO = aznew AZ::IO::LocalFileIO();
            m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            // we need to set it to nullptr first because otherwise the 
            // underneath code assumes that we might be leaking the previous instance
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_localFileIO);
            m_tempDir = new UnitTest::ScopedTemporaryDirectory();
        }

        void TearDown() override
        {
            delete m_tempDir;
            AZ::IO::FileIOBase::SetInstance(nullptr);
            delete m_localFileIO;
            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
            AzFramework::ApplicationRequests::Bus::Handler::BusDisconnect();
            ScopedAllocatorSetupFixture::TearDown();
        }

        // AzFramework::ApplicationRequests::Bus::Handler interface
        void NormalizePath(AZStd::string& /*path*/) override {}
        void NormalizePathKeepCase(AZStd::string& /*path*/) override {}
        void CalculateBranchTokenForEngineRoot(AZStd::string& /*token*/) const override {}

        const char* GetAppRoot() const  override
        {
            return m_tempDir->GetDirectory();
        }

        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZ::IO::FileIOBase* m_localFileIO = nullptr;
        UnitTest::ScopedTemporaryDirectory* m_tempDir = nullptr;
    };


    TEST_F(MockUtilsTest, DISABLED_TestFilePath_StartsWithAFileSeparator_Valid)
    {
        AZStd::string relFilePath = "Foo/foo.xml";
        AzFramework::StringFunc::Prepend(relFilePath, AZ_CORRECT_FILESYSTEM_SEPARATOR);
        AZStd::string absoluteFilePath;
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        AZStd::string driveString;
        AzFramework::StringFunc::Path::GetDrive(GetAppRoot(), driveString);
        AzFramework::StringFunc::Path::ConstructFull(driveString.c_str(), relFilePath.c_str(), absoluteFilePath, true);
#else
        absoluteFilePath = relFilePath;
#endif
        FilePath filePath(relFilePath);
        EXPECT_STREQ(filePath.AbsolutePath().c_str(), absoluteFilePath.c_str());
    }


    TEST_F(MockUtilsTest, TestFilePath_RelativePath_Valid)
    {
        AZStd::string relFilePath = "Foo\\foo.xml";
        AZStd::string absoluteFilePath;
        AzFramework::StringFunc::Path::ConstructFull(GetAppRoot(), relFilePath.c_str(), absoluteFilePath, true);
        FilePath filePath(relFilePath);
        EXPECT_EQ(filePath.AbsolutePath(), absoluteFilePath);
    }

    TEST_F(MockUtilsTest, TestFilePath_CasingMismatch_Error_valid)
    {
        AZStd::string relFilePath = "Foo\\Foo.xml";
        AZStd::string wrongCaseRelFilePath = "Foo\\foo.xml";
        AZStd::string correctAbsoluteFilePath;
        AZStd::string wrongCaseAbsoluteFilePath;
        AzFramework::StringFunc::Path::ConstructFull(GetAppRoot(), relFilePath.c_str(), correctAbsoluteFilePath, true);
        AzFramework::StringFunc::Path::ConstructFull(GetAppRoot(), wrongCaseRelFilePath.c_str(), wrongCaseAbsoluteFilePath, true);
        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        AZ::IO::FileIOBase::GetInstance()->Open(correctAbsoluteFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath, fileHandle);
        FilePath filePath(wrongCaseAbsoluteFilePath, true, false);
        EXPECT_FALSE(filePath.IsValid());
        EXPECT_TRUE(filePath.ErrorString().find("File case mismatch") != AZStd::string::npos);
    }

    TEST_F(MockUtilsTest, TestFilePath_NoFileExists_NoError_valid)
    {
        AZStd::string relFilePath = "Foo\\Foo.xml";
        AZStd::string absoluteFilePath;
        AzFramework::StringFunc::Path::ConstructFull(GetAppRoot(), relFilePath.c_str(), absoluteFilePath, true);
        FilePath filePath(absoluteFilePath, true, false);
        EXPECT_TRUE(filePath.IsValid());
        EXPECT_TRUE(filePath.ErrorString().empty());
    }

    TEST_F(MockUtilsTest, TestFilePath_CasingMismatch_Ignore_Filecase_valid)
    {
        AZStd::string relFilePath = "Foo\\Foo.xml";
        AZStd::string wrongCaseRelFilePath = "Foo\\foo.xml";
        AZStd::string correctAbsoluteFilePath;
        AZStd::string wrongCaseAbsoluteFilePath;
        AzFramework::StringFunc::Path::ConstructFull(GetAppRoot(), relFilePath.c_str(), correctAbsoluteFilePath, true);
        AzFramework::StringFunc::Path::ConstructFull(GetAppRoot(), wrongCaseRelFilePath.c_str(), wrongCaseAbsoluteFilePath, true);
        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        AZ::IO::FileIOBase::GetInstance()->Open(correctAbsoluteFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath, fileHandle);
        FilePath filePath(wrongCaseAbsoluteFilePath, true, true);
        EXPECT_TRUE(filePath.IsValid());
        EXPECT_STREQ(filePath.AbsolutePath().c_str(), correctAbsoluteFilePath.c_str());
    }

    TEST_F(MockUtilsTest, LooksLikeWildcardPattern_IsWildcardPattern_ExpectTrue)
    {
        EXPECT_TRUE(LooksLikeWildcardPattern("*"));
        EXPECT_TRUE(LooksLikeWildcardPattern("?"));
        EXPECT_TRUE(LooksLikeWildcardPattern("*/*"));
        EXPECT_TRUE(LooksLikeWildcardPattern("*/test?/*.xml"));
    }

    TEST_F(MockUtilsTest, LooksLikeWildcardPattern_IsNotWildcardPattern_ExpectFalse)
    {
        EXPECT_FALSE(LooksLikeWildcardPattern(""));
        EXPECT_FALSE(LooksLikeWildcardPattern("test"));
        EXPECT_FALSE(LooksLikeWildcardPattern("test/path.xml"));
    }
}
