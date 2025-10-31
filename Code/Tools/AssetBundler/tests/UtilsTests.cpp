/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/API/ApplicationAPI.h>
#include <source/utils/utils.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Utils/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>


namespace AssetBundler
{
    class MockUtilsTest
        : public UnitTest::LeakDetectionFixture
        , public AzFramework::ApplicationRequests::Bus::Handler
    {
    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            AzFramework::ApplicationRequests::Bus::Handler::BusConnect();
            m_localFileIO = aznew AZ::IO::LocalFileIO();
            m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            // we need to set it to nullptr first because otherwise the
            // underneath code assumes that we might be leaking the previous instance
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_localFileIO);
            m_tempDir = new AZ::Test::ScopedAutoTempDirectory();
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry == nullptr)
            {
                m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
                settingsRegistry = m_settingsRegistry.get();
                AZ::SettingsRegistry::Register(settingsRegistry);
            }
            settingsRegistry->Get(m_oldEngineRoot.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
            settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder, m_tempDir->GetDirectory());
        }

        void TearDown() override
        {
            // Reset Engine Path if there was an existing Settings Registry from before
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder, m_oldEngineRoot.Native());
            if(settingsRegistry == m_settingsRegistry.get())
            {
                AZ::SettingsRegistry::Unregister(settingsRegistry);
                m_settingsRegistry.reset();
            }
            delete m_tempDir;
            AZ::IO::FileIOBase::SetInstance(nullptr);
            delete m_localFileIO;
            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
            AzFramework::ApplicationRequests::Bus::Handler::BusDisconnect();
            LeakDetectionFixture::TearDown();
        }

        // AzFramework::ApplicationRequests::Bus::Handler interface
        void NormalizePath(AZStd::string& /*path*/) override {}
        void NormalizePathKeepCase(AZStd::string& /*path*/) override {}
        void CalculateBranchTokenForEngineRoot(AZStd::string& /*token*/) const override {}

        const char* GetTempDir() const
        {
            return m_tempDir->GetDirectory();
        }

        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZ::IO::FileIOBase* m_localFileIO = nullptr;
        AZ::Test::ScopedAutoTempDirectory* m_tempDir = nullptr;
        AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;
        AZ::IO::Path m_oldEngineRoot;
    };


    TEST_F(MockUtilsTest, DISABLED_TestFilePath_StartsWithAFileSeparator_Valid)
    {
        AZ::IO::Path relFilePath = "Foo/foo.xml";
        AZ::IO::Path absoluteFilePath = AZ::IO::PathView(GetTempDir()).RootPath();
        absoluteFilePath /= relFilePath;
        absoluteFilePath = absoluteFilePath.LexicallyNormal();

        FilePath filePath(relFilePath.Native());
        EXPECT_STREQ(filePath.AbsolutePath().c_str(), absoluteFilePath.c_str());
    }


    TEST_F(MockUtilsTest, TestFilePath_RelativePath_Valid)
    {
        AZ::IO::Path relFilePath = "Foo\\foo.xml";
        AZ::IO::Path absoluteFilePath = (AZ::IO::Path(GetTempDir()) / relFilePath).LexicallyNormal();
        FilePath filePath(relFilePath.Native());
        EXPECT_EQ(AZ::IO::PathView{ filePath.AbsolutePath() }, absoluteFilePath);
    }

#if !AZ_TRAIT_USE_WINDOWS_FILE_API
    // When using Windows file API the the AZ::IO::Path comparisons are case insensitive
    TEST_F(MockUtilsTest, TestFilePath_CasingMismatch_Error_valid)
    {
        AZ::IO::Path relFilePath = "Foo\\Foo.xml";
        AZ::IO::Path wrongCaseRelFilePath = "Foo\\foo.xml";

        AZ::IO::Path correctAbsoluteFilePath = (AZ::IO::Path(GetTempDir()) / relFilePath).LexicallyNormal();
        AZ::IO::Path wrongCaseAbsoluteFilePath = (AZ::IO::Path(GetTempDir()) / wrongCaseRelFilePath).LexicallyNormal();

        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        AZ::IO::FileIOBase::GetInstance()->Open(correctAbsoluteFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath, fileHandle);
        FilePath filePath(wrongCaseAbsoluteFilePath.Native(), true, false);
        EXPECT_FALSE(filePath.IsValid());
        EXPECT_TRUE(filePath.ErrorString().contains("File case mismatch"));
    }
#endif

    TEST_F(MockUtilsTest, TestFilePath_NoFileExists_NoError_valid)
    {
        AZ::IO::Path relFilePath = "Foo\\Foo.xml";
        AZ::IO::Path absoluteFilePath = (AZ::IO::Path(GetTempDir()) / relFilePath).LexicallyNormal();

        FilePath filePath(absoluteFilePath.Native(), true, false);
        EXPECT_TRUE(filePath.IsValid());
        EXPECT_TRUE(filePath.ErrorString().empty());
    }

    TEST_F(MockUtilsTest, TestFilePath_CasingMismatch_Ignore_Filecase_valid)
    {
        AZStd::string relFilePath = "Foo\\Foo.xml";
        AZStd::string wrongCaseRelFilePath = "Foo\\foo.xml";
        AZ::IO::Path correctAbsoluteFilePath = (AZ::IO::Path(GetTempDir()) / relFilePath).LexicallyNormal();
        AZ::IO::Path wrongCaseAbsoluteFilePath = (AZ::IO::Path(GetTempDir()) / wrongCaseRelFilePath).LexicallyNormal();

        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        AZ::IO::FileIOBase::GetInstance()->Open(correctAbsoluteFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath, fileHandle);
        FilePath filePath(wrongCaseAbsoluteFilePath.Native(), true, true);
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
