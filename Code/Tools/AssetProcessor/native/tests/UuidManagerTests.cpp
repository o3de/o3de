/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/Metadata/MetadataManager.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Utils/Utils.h>
#include <native/utilities/UuidManager.h>
#include <AssetManager/SourceAssetReference.h>
#include <tests/UnitTestUtilities.h>
#include <Tests/AZTestShared/Utils/Utils.h>
#include <unittests/UnitTestUtils.h>

namespace UnitTests
{
    struct UuidManagerTests
        : UnitTest::LeakDetectionFixture
    {
        void SetUp() override
        {
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
            m_componentApplication = AZStd::make_unique<testing::NiceMock<MockComponentApplication>>();

            using namespace testing;

            ON_CALL(*m_componentApplication.get(), GetSerializeContext()).WillByDefault(Return(m_serializeContext.get()));
            ON_CALL(*m_componentApplication.get(), GetJsonRegistrationContext()).WillByDefault(Return(m_jsonRegistrationContext.get()));

            AZ::JsonSystemComponent::Reflect(m_jsonRegistrationContext.get());

            AssetProcessor::UuidManager::Reflect(m_serializeContext.get());

            m_uuidInterface = AZ::Interface<AssetProcessor::IUuidRequests>::Get();

            ASSERT_TRUE(m_uuidInterface);

            // Enable txt files by default for these tests
            m_uuidInterface->EnableGenerationForTypes({ ".txt" });
        }

        void TearDown() override
        {
            m_jsonRegistrationContext->EnableRemoveReflection();
            AZ::JsonSystemComponent::Reflect(m_jsonRegistrationContext.get());
            m_jsonRegistrationContext->DisableRemoveReflection();

            m_jsonRegistrationContext.reset();
            m_serializeContext.reset();
        }

        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;

        MockFileStateCache m_fileStateCache;
        AzToolsFramework::MetadataManager m_metadataManager;
        AssetProcessor::UuidManager m_uuidManager;
        MockPathConversion m_pathConversion;
        MockVirtualFileIO m_virtualFileIO;
        AZStd::unique_ptr<testing::NiceMock<MockComponentApplication>> m_componentApplication;

        AssetProcessor::IUuidRequests* m_uuidInterface{};
    };

    void MakeFile(AZ::IO::PathView path)
    {
        ASSERT_TRUE(UnitTestUtils::CreateDummyFileAZ(path));
    }

    TEST_F(UuidManagerTests, GetUuid_FirstTime_ReturnsRandomUuid)
    {
        static constexpr const char* TestFile = "c:/somepath/mockfile.txt";

        MakeFile(TestFile);

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_FALSE(uuid.IsNull());
        // Make sure a metadata file was created
        EXPECT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(AZStd::string::format("%s%s", TestFile, AzToolsFramework::MetadataManager::MetadataFileExtension).c_str()));
    }

    TEST_F(UuidManagerTests, GetUuid_FileDoesNotExist_Fails)
    {
        static constexpr const char* TestFile = "c:/somepath/mockfile.txt";

        TraceBusErrorChecker errorHandler;
        errorHandler.Begin();
        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));
        errorHandler.End(1);

        EXPECT_TRUE(uuid.IsNull());
    }

    TEST_F(UuidManagerTests, GetUuid_ExistingFileDeleted_Fails)
    {
        static constexpr const char* TestFile = "c:/somepath/mockfile.txt";

        MakeFile(TestFile);

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_FALSE(uuid.IsNull());
        // Make sure a metadata file was created
        EXPECT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(
            AZStd::string::format("%s%s", TestFile, AzToolsFramework::MetadataManager::MetadataFileExtension).c_str()));

        // Remove the file
        AZ::IO::FileIOBase::GetInstance()->Remove(TestFile);
        AZ::IO::FileIOBase::GetInstance()->Remove(
            (AZStd::string(TestFile) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());
        m_uuidInterface->FileRemoved((AZStd::string(TestFile) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        // Check the UUID again, expecting an error
        TraceBusErrorChecker errorHandler;
        errorHandler.Begin();
        uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));
        errorHandler.End(1);

        EXPECT_TRUE(uuid.IsNull());
    }

    TEST_F(UuidManagerTests, GetUuidTwice_ReturnsSameUuid)
    {
        static constexpr const char* TestFile = "c:/somepath/Mockfile.txt";

        MakeFile(TestFile);

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_FALSE(uuid.IsNull());

        auto uuid2 = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_EQ(uuid, uuid2);
    }

    TEST_F(UuidManagerTests, GetUuid_DifferentFiles_ReturnsDifferentUuid)
    {
        static constexpr const char* FileA = "c:/somepath/fileA.txt";
        static constexpr const char* FileB = "c:/somepath/fileB.txt";

        MakeFile(FileA);
        MakeFile(FileB);

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileA));

        EXPECT_FALSE(uuid.IsNull());

        auto uuid2 = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileB));

        EXPECT_NE(uuid, uuid2);
    }

    TEST_F(UuidManagerTests, GetLegacyUuids_UppercaseFileName_ReturnsTwoDifferentUuids)
    {
        static constexpr const char* TestFile = "c:/somepath/Mockfile.txt";

        MakeFile(TestFile);

        auto uuids = m_uuidInterface->GetLegacyUuids(AssetProcessor::SourceAssetReference(TestFile));

        ASSERT_EQ(uuids.size(), 2);
        EXPECT_NE(*uuids.begin(), *++uuids.begin());
    }

    TEST_F(UuidManagerTests, GetLegacyUuids_LowercaseFileName_ReturnsOneUuid)
    {
        static constexpr const char* TestFile = "c:/somepath/mockfile.txt";

        MakeFile(TestFile);

        auto uuids = m_uuidInterface->GetLegacyUuids(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_EQ(uuids.size(), 1);
    }

    TEST_F(UuidManagerTests, GetLegacyUuids_DifferentFromCanonicalUuid)
    {
        static constexpr const char* TestFile = "c:/somepath/Mockfile.txt";

        MakeFile(TestFile);

        auto legacyUuids = m_uuidInterface->GetLegacyUuids(AssetProcessor::SourceAssetReference(TestFile));

        ASSERT_EQ(legacyUuids.size(), 2);

        auto canonicalUuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_THAT(legacyUuids, ::testing::Not(::testing::Contains(canonicalUuid)));
    }

    TEST_F(UuidManagerTests, MoveFile_UuidRemainsTheSame)
    {
        static constexpr const char* FileA = "c:/somepath/mockfile.txt";
        static constexpr const char* FileB = "c:/somepath/newfile.txt";

        MakeFile(FileA);
        MakeFile(FileB);

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileA));

        // Move the metadata file and signal the old one is removed
        AZ::IO::FileIOBase::GetInstance()->Rename(
            (AZStd::string(FileA) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str(),
            (AZStd::string(FileB) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        m_uuidInterface->FileRemoved((AZStd::string(FileA) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        auto movedUuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileB));

        EXPECT_EQ(uuid, movedUuid);
    }

    TEST_F(UuidManagerTests, MoveFileWithComplexName_UuidRemainsTheSame)
    {
        static constexpr const char* FileA = "c:/somepath/mockfile.ext1.ext2.txt";
        static constexpr const char* FileB = "c:/somepath/newfile.txt";

        MakeFile(FileA);
        MakeFile(FileB);

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileA));

        // Move the metadata file and signal the old one is removed
        AZ::IO::FileIOBase::GetInstance()->Rename(
            (AZStd::string(FileA) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str(),
            (AZStd::string(FileB) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        m_uuidInterface->FileRemoved((AZStd::string(FileA) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        auto movedUuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileB));

        EXPECT_EQ(uuid, movedUuid);
    }

    TEST_F(UuidManagerTests, MetadataRemoved_NewUuidAssigned)
    {
        static constexpr const char* TestFile = "c:/somepath/mockfile.txt";

        MakeFile(TestFile);

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        // Delete the metadata file and signal its removal
        AZ::IO::FileIOBase::GetInstance()->Remove(
            (AZStd::string(TestFile) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        m_uuidInterface->FileRemoved((AZStd::string(TestFile) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        auto movedUuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_NE(uuid, movedUuid);
    }

    TEST_F(UuidManagerTests, MetadataUpdated_NewUuidAssigned)
    {
        static constexpr const char* FileA = "c:/somepath/mockfile.test.txt";
        static constexpr const char* FileB = "c:/somepath/someotherfile.txt";

        MakeFile(FileA);
        MakeFile(FileB);

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileA));

        // Generate another metadata file, its the easiest way to "change" a UUID in the metadata file for this test
        m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileB));

        AZ::IO::FileIOBase::GetInstance()->Remove(
            (AZStd::string(FileA) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        // Copy FileB's metadata onto the FileA metadata
        AZ::IO::FileIOBase::GetInstance()->Rename(
            (AZStd::string(FileB) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str(),
            (AZStd::string(FileA) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        m_uuidInterface->FileChanged((AZStd::string(FileA) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        auto newUuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileA));

        EXPECT_NE(uuid, newUuid);
    }

    TEST_F(UuidManagerTests, RequestUuid_DisabledType_ReturnsLegacyUuid)
    {
        static constexpr const char* TestFile = "c:/somepath/mockfile.png";

        MakeFile(TestFile);

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));
        auto legacyUuids = m_uuidInterface->GetLegacyUuids(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_THAT(legacyUuids, ::testing::Contains(uuid));

        // Make sure no metadata file was created
        EXPECT_FALSE(AZ::IO::FileIOBase::GetInstance()->Exists(
            AZStd::string::format("%s%s", TestFile, AzToolsFramework::MetadataManager::MetadataFileExtension).c_str()));
    }

    TEST_F(UuidManagerTests, TwoFilesWithSameRelativePath_DisabledType_ReturnsSameUuid)
    {
        static constexpr AZ::IO::FixedMaxPath FileA = "c:/somepath/folderA/mockfile.png"; // png files are disabled
        static constexpr AZ::IO::FixedMaxPath FileB = "c:/somepath/folderB/mockfile.png";

        MakeFile(FileA);
        MakeFile(FileB);

        auto uuidA = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(1, FileA.ParentPath(), FileA.Filename()));
        auto uuidB = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(2, FileB.ParentPath(), FileB.Filename()));

        EXPECT_FALSE(uuidA.IsNull());
        EXPECT_FALSE(uuidB.IsNull());
        EXPECT_EQ(uuidA, uuidB);
    }

    TEST_F(UuidManagerTests, TwoFilesWithSameRelativePath_EnabledType_ReturnsDifferentUuid)
    {
        static constexpr AZ::IO::FixedMaxPath FileA = "c:/somepath/folderA/mockfile.txt"; // txt files are enabled
        static constexpr AZ::IO::FixedMaxPath FileB = "c:/somepath/folderB/mockfile.txt";

        MakeFile(FileA);
        MakeFile(FileB);

        auto uuidA = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(1, FileA.ParentPath(), FileA.Filename()));
        auto uuidB = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(2, FileB.ParentPath(), FileB.Filename()));

        EXPECT_FALSE(uuidA.IsNull());
        EXPECT_FALSE(uuidB.IsNull());
        EXPECT_NE(uuidA, uuidB);
    }

    TEST_F(UuidManagerTests, GetUuid_CorruptedFile_Fails)
    {
        static constexpr AZ::IO::FixedMaxPath TestFile = "c:/somepath/mockfile.txt";
        static constexpr AZ::IO::FixedMaxPath MetadataFile = TestFile.Native() + AzToolsFramework::MetadataManager::MetadataFileExtension;

        MakeFile(TestFile);

        // Generate a metadata file
        m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        // Read in the metadata file
        auto result = AZ::Utils::ReadFile<AZStd::string>(MetadataFile.Native());

        ASSERT_TRUE(result.IsSuccess());

        auto metadataFileContents = result.GetValue();

        // Corrupt the first character of the metadata file and write it back to disk, signalling a file change as well
        metadataFileContents.data()[0] = 'A';
        AZ::Utils::WriteFile(metadataFileContents, MetadataFile.Native());
        m_uuidInterface->FileChanged(MetadataFile);

        // Try to read the metadata again, expecting an error
        TraceBusErrorChecker errorChecker;
        errorChecker.Begin();
        auto uuidRetry = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));
        errorChecker.End(2);

        EXPECT_TRUE(uuidRetry.IsNull());
    }
}
