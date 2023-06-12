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

            AzToolsFramework::UuidUtilComponent::Reflect(m_serializeContext.get());
            AzToolsFramework::MetadataManager::Reflect(m_serializeContext.get());
            AssetProcessor::UuidManager::Reflect(m_serializeContext.get());

            m_uuidInterface = AZ::Interface<AssetProcessor::IUuidRequests>::Get();

            ASSERT_TRUE(m_uuidInterface);

            // Enable txt files by default for these tests
            m_uuidInterface->EnableGenerationForTypes({ ".txt" });

            m_pathConversion.AddScanfolder("c:/somepath", "somepath");
            m_pathConversion.AddScanfolder("c:/other", "other");
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
        AzToolsFramework::UuidUtilComponent m_uuidUtil;
        AzToolsFramework::MetadataManager m_metadataManager;
        AssetProcessor::UuidManager m_uuidManager;
        MockMultiPathConversion m_pathConversion;
        MockVirtualFileIO m_virtualFileIO;
        AZStd::unique_ptr<testing::NiceMock<MockComponentApplication>> m_componentApplication;
        TraceBusErrorChecker m_errorChecker;

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

        EXPECT_TRUE(uuid);
        // Make sure a metadata file was created
        EXPECT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(AZStd::string::format("%s%s", TestFile, AzToolsFramework::MetadataManager::MetadataFileExtension).c_str()));
    }

    TEST_F(UuidManagerTests, GetUuid_FileDoesNotExist_Fails)
    {
        static constexpr const char* TestFile = "c:/somepath/mockfile.txt";

        m_errorChecker.Begin();
        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));
        m_errorChecker.End(1);

        EXPECT_FALSE(uuid);
    }

    TEST_F(UuidManagerTests, GetUuid_ExistingFileDeleted_Fails)
    {
        static constexpr const char* TestFile = "c:/somepath/mockfile.txt";

        MakeFile(TestFile);

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_TRUE(uuid);
        // Make sure a metadata file was created
        EXPECT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(
            AZStd::string::format("%s%s", TestFile, AzToolsFramework::MetadataManager::MetadataFileExtension).c_str()));

        // Remove the file
        AZ::IO::FileIOBase::GetInstance()->Remove(TestFile);
        AZ::IO::FileIOBase::GetInstance()->Remove(
            (AZStd::string(TestFile) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());
        m_uuidInterface->FileRemoved((AZStd::string(TestFile) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        // Check the UUID again, expecting an error
        m_errorChecker.Begin();
        uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));
        m_errorChecker.End(1);

        EXPECT_FALSE(uuid);
    }

    TEST_F(UuidManagerTests, GetUuidTwice_ReturnsSameUuid)
    {
        static constexpr const char* TestFile = "c:/somepath/Mockfile.txt";

        MakeFile(TestFile);

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        ASSERT_TRUE(uuid);

        auto uuid2 = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        ASSERT_TRUE(uuid2);
        EXPECT_EQ(uuid.GetValue(), uuid2.GetValue());
    }

    TEST_F(UuidManagerTests, GetUuid_DifferentFiles_ReturnsDifferentUuid)
    {
        static constexpr const char* FileA = "c:/somepath/fileA.txt";
        static constexpr const char* FileB = "c:/somepath/fileB.txt";

        MakeFile(FileA);
        MakeFile(FileB);

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileA));

        ASSERT_TRUE(uuid);

        auto uuid2 = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileB));

        ASSERT_TRUE(uuid2);
        EXPECT_NE(uuid.GetValue(), uuid2.GetValue());
    }

    TEST_F(UuidManagerTests, GetLegacyUuids_UppercaseFileName_ReturnsTwoDifferentUuids)
    {
        static constexpr const char* TestFile = "c:/somepath/Mockfile.txt";

        MakeFile(TestFile);

        auto result = m_uuidInterface->GetLegacyUuids(AssetProcessor::SourceAssetReference(TestFile));

        ASSERT_TRUE(result);

        auto uuids = result.GetValue();

        ASSERT_EQ(uuids.size(), 2);
        EXPECT_NE(*uuids.begin(), *++uuids.begin());
    }

    TEST_F(UuidManagerTests, GetLegacyUuids_LowercaseFileName_ReturnsOneUuid)
    {
        static constexpr const char* TestFile = "c:/somepath/mockfile.txt";

        MakeFile(TestFile);

        auto result = m_uuidInterface->GetLegacyUuids(AssetProcessor::SourceAssetReference(TestFile));

        ASSERT_TRUE(result);
        EXPECT_EQ(result.GetValue().size(), 1);
    }

    TEST_F(UuidManagerTests, GetLegacyUuids_DifferentFromCanonicalUuid)
    {
        static constexpr const char* TestFile = "c:/somepath/Mockfile.txt";

        MakeFile(TestFile);

        auto legacyUuids = m_uuidInterface->GetLegacyUuids(AssetProcessor::SourceAssetReference(TestFile));

        ASSERT_TRUE(legacyUuids);
        ASSERT_EQ(legacyUuids.GetValue().size(), 2);

        auto canonicalUuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        ASSERT_TRUE(canonicalUuid);
        EXPECT_THAT(legacyUuids.GetValue(), ::testing::Not(::testing::Contains(canonicalUuid.GetValue())));
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

        ASSERT_TRUE(uuid);
        ASSERT_TRUE(movedUuid);

        EXPECT_EQ(uuid.GetValue(), movedUuid.GetValue());
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

        ASSERT_TRUE(uuid);
        ASSERT_TRUE(movedUuid);

        EXPECT_EQ(uuid.GetValue(), movedUuid.GetValue());
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

        ASSERT_TRUE(uuid);
        ASSERT_TRUE(movedUuid);

        EXPECT_NE(uuid.GetValue(), movedUuid.GetValue());
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

        m_uuidInterface->FileRemoved((AZStd::string(FileB) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());
        m_uuidInterface->FileChanged((AZStd::string(FileA) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        auto newUuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileA));

        ASSERT_TRUE(uuid);
        ASSERT_TRUE(newUuid);

        EXPECT_NE(uuid.GetValue(), newUuid.GetValue());
    }

    TEST_F(UuidManagerTests, RequestUuid_DisabledType_ReturnsLegacyUuid)
    {
        static constexpr const char* TestFile = "c:/somepath/mockfile.png";

        MakeFile(TestFile);

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));
        auto legacyUuids = m_uuidInterface->GetLegacyUuids(AssetProcessor::SourceAssetReference(TestFile));

        ASSERT_TRUE(uuid);
        ASSERT_TRUE(legacyUuids);
        EXPECT_THAT(legacyUuids.GetValue(), ::testing::Contains(uuid.GetValue()));

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

        ASSERT_TRUE(uuidA);
        ASSERT_TRUE(uuidB);
        EXPECT_EQ(uuidA.GetValue(), uuidB.GetValue());
    }

    TEST_F(UuidManagerTests, TwoFilesWithSameRelativePath_EnabledType_ReturnsDifferentUuid)
    {
        static constexpr AZ::IO::FixedMaxPath FileA = "c:/somepath/folderA/mockfile.txt"; // txt files are enabled
        static constexpr AZ::IO::FixedMaxPath FileB = "c:/somepath/folderB/mockfile.txt";

        MakeFile(FileA);
        MakeFile(FileB);

        auto uuidA = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(1, FileA.ParentPath(), FileA.Filename()));
        auto uuidB = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(2, FileB.ParentPath(), FileB.Filename()));

        ASSERT_TRUE(uuidA);
        ASSERT_TRUE(uuidB);
        EXPECT_NE(uuidA.GetValue(), uuidB.GetValue());
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
        auto uuidRetry = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_FALSE(uuidRetry);
    }

    TEST_F(UuidManagerTests, GetUuid_IncompleteMetadataFile_ReturnsAndUpdates)
    {
        static constexpr AZ::IO::FixedMaxPath TestFile = "c:/somepath/mockfile.txt";
        static constexpr AZ::IO::FixedMaxPath MetadataFile = TestFile.Native() + AzToolsFramework::MetadataManager::MetadataFileExtension;

        MakeFile(TestFile);

        static constexpr AZ::Uuid testUuid{ "{2EE0C7C2-F21E-4254-A180-174992819254}" };
        AZStd::string contents = AZStd::string::format("{\"UUID\": {\"uuid\": \"%s\"}}", testUuid.ToFixedString().c_str());

        AZ::Utils::WriteFile(contents, MetadataFile.Native());

        auto uuidRetry = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        ASSERT_TRUE(uuidRetry);
        EXPECT_EQ(uuidRetry.GetValue(), testUuid);

        auto legacyIds = m_uuidInterface->GetLegacyUuids(AssetProcessor::SourceAssetReference(TestFile));

        ASSERT_TRUE(legacyIds);
        EXPECT_EQ(legacyIds.GetValue().size(), 1);
    }

    TEST_F(UuidManagerTests, GetUuid_MetadataFileNoUuid_Fails)
    {
        static constexpr AZ::IO::FixedMaxPath TestFile = "c:/somepath/mockfile.txt";
        static constexpr AZ::IO::FixedMaxPath MetadataFile = TestFile.Native() + AzToolsFramework::MetadataManager::MetadataFileExtension;

        MakeFile(TestFile);

        AZStd::string contents = AZStd::string::format("{\"UUID\": {\"originalPath\": \" " AZ_STRING_FORMAT " \"}}", AZ_STRING_ARG(TestFile.Filename().Native()));

        AZ::Utils::WriteFile(contents, MetadataFile.Native());

        auto uuidRetry = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_FALSE(uuidRetry);
    }

    TEST_F(UuidManagerTests, GetUuid_DuplicateUuids_Fails)
    {
        static constexpr AZ::IO::FixedMaxPath FileA = "c:/somepath/mockfile.test.txt";
        static constexpr AZ::IO::FixedMaxPath FileB = "c:/somepath/someotherfile.txt";

        MakeFile(FileA);
        MakeFile(FileB);

        auto uuidA = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileA));

        // Assign the same UUID to FileB (note this doesn't fail)
        AZ::Interface<AzToolsFramework::IUuidUtil>::Get()->CreateSourceUuid(FileB, uuidA.GetValue());

        auto uuidB = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileB));

        EXPECT_FALSE(uuidB);
    }

    TEST_F(UuidManagerTests, GetUuid_DuplicateUuidsClearedCache_Succeeds)
    {
        static constexpr AZ::IO::FixedMaxPath FileA = "c:/somepath/mockfile.test.txt";
        static constexpr AZ::IO::FixedMaxPath FileB = "c:/somepath/someotherfile.txt";

        MakeFile(FileA);
        MakeFile(FileB);

        auto uuidA = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileA));

        // Assign the same UUID to FileB
        AZ::Interface<AzToolsFramework::IUuidUtil>::Get()->CreateSourceUuid(FileB, uuidA.GetValue());

        // Pretend we deleted FileA so there shouldn't be a conflict anymore
        m_uuidInterface->FileRemoved(AzToolsFramework::MetadataManager::ToMetadataPath(FileA.c_str()));

        auto uuidB = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileB));

        ASSERT_TRUE(uuidA);
        ASSERT_TRUE(uuidB);

        EXPECT_EQ(uuidB.GetValue(), uuidA.GetValue());
    }

    TEST_F(UuidManagerTests, UpdateCase)
    {
        static constexpr AZ::IO::FixedMaxPath TestFile = "c:/somepath/mockfile.txt";
        static constexpr AZ::IO::FixedMaxPath RenamedFile = "c:/somepath/MockFile.txt";

        MakeFile(TestFile);

        // Generate the metadata file
        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_TRUE(uuid);

        auto* io = AZ::IO::FileIOBase::GetInstance();

        // Make sure the metadata file exists (this is not a case sensitive check)
        EXPECT_TRUE(io->Exists(AzToolsFramework::MetadataManager::ToMetadataPath(TestFile.c_str()).c_str()));

        QString relPath = "mockfile.txt";
        relPath += AzToolsFramework::MetadataManager::MetadataFileExtension;

        // Verify the case of the metadata file is lowercase to start with
        EXPECT_TRUE(AssetUtilities::UpdateToCorrectCase("c:/somepath", relPath));
        EXPECT_STREQ(relPath.toUtf8().constData(), (QString("mockfile.txt") + AzToolsFramework::MetadataManager::MetadataFileExtension).toUtf8().constData());

        // Rename the source file from lowercase to uppercase and notify about the old file being removed
        io->Rename(TestFile.c_str(), RenamedFile.c_str());
        m_uuidInterface->FileRemoved(TestFile);

        // Request the UUID again, this should automatically update the case of the metadata file
        m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(RenamedFile));

        // Verify the metadata file exists (this is not a case sensitive check)
        EXPECT_TRUE(io->Exists(AzToolsFramework::MetadataManager::ToMetadataPath(RenamedFile.c_str()).c_str()));

        // Verify the case of the metadata file is actually updated
        EXPECT_TRUE(AssetUtilities::UpdateToCorrectCase("c:/somepath", relPath));
        EXPECT_STREQ(relPath.toUtf8().constData(), (QString("MockFile.txt") + AzToolsFramework::MetadataManager::MetadataFileExtension).toUtf8().constData());
    }

    TEST_F(UuidManagerTests, FindFilesByUuid)
    {
        // Test that FindFilesByUuid correctly returns all files with the same matching (legacy) UUID
        using namespace AssetProcessor;

        static constexpr AZ::IO::FixedMaxPath FileA = "c:/somepath/mockfile.txt";
        static constexpr AZ::IO::FixedMaxPath FileB = "c:/other/MockFile.txt";
        static constexpr AZ::IO::FixedMaxPath FileC = "c:/other/notvalid/mockFile.txt"; // throw in a random extra file to make sure only matching files are returned

        MakeFile(FileA);
        MakeFile(FileB);
        MakeFile(FileC);

        auto fileALegacy = m_uuidInterface->GetLegacyUuids(SourceAssetReference(FileA));
        auto fileBLegacy = m_uuidInterface->GetLegacyUuids(SourceAssetReference(FileB));

        ASSERT_TRUE(fileALegacy);
        ASSERT_TRUE(fileBLegacy);

        auto fileAUuid = *fileALegacy.GetValue().begin();

        EXPECT_TRUE(fileBLegacy.GetValue().contains(fileAUuid));

        auto files = m_uuidInterface->FindFilesByUuid(fileAUuid);

        EXPECT_THAT(files, ::testing::UnorderedElementsAre(FileA, FileB));

        auto fileACanonical = m_uuidInterface->GetUuid(SourceAssetReference(FileA));

        ASSERT_TRUE(fileACanonical);

        EXPECT_THAT(m_uuidInterface->FindFilesByUuid(fileACanonical.GetValue()), ::testing::UnorderedElementsAre(FileA));
    }

    TEST_F(UuidManagerTests, FindHighestPriorityFileByUuid)
    {
        // Test that FindHighestPriorityFileByUuid returns the oldest file
        using namespace AssetProcessor;

        static constexpr AZ::IO::FixedMaxPath FileA = "c:/somepath/fileA.txt";
        static constexpr AZ::IO::FixedMaxPath FileB = "c:/somepath/fileB.txt";
        static constexpr AZ::IO::FixedMaxPath FileC = "c:/other/fileA.txt";
        static constexpr AZ::IO::FixedMaxPath FileD = "c:/other/fileB.txt";

        MakeFile(FileA);
        MakeFile(FileB);
        MakeFile(FileC);
        MakeFile(FileD);

        auto result = m_uuidInterface->GetUuidDetails(AssetProcessor::SourceAssetReference(FileA));

        ASSERT_TRUE(result);

        auto uuidDetails = result.GetValue();

        // Copy the metadata for FileA but give it a different canonical UUID and an older timestamp
        // This is really just meant to duplicate the legacy UUIDs
        uuidDetails.m_uuid = AZ::Uuid::CreateRandom();
        uuidDetails.m_millisecondsSinceUnixEpoch = uuidDetails.m_millisecondsSinceUnixEpoch - 1;
        m_metadataManager.SetValue(FileB, AzToolsFramework::UuidUtilComponent::UuidKey, &uuidDetails, azrtti_typeid<decltype(uuidDetails)>());

        // Get UUID manager to load the uuids
        m_uuidInterface->GetUuid(SourceAssetReference(FileB));
        m_uuidInterface->GetUuid(SourceAssetReference(FileC));
        m_uuidInterface->GetUuid(SourceAssetReference(FileD));

        auto highestPriority = m_uuidInterface->FindHighestPriorityFileByUuid(*uuidDetails.m_legacyUuids.begin());

        ASSERT_TRUE(highestPriority);

        EXPECT_EQ(highestPriority.value(), FileB);
    }

    TEST_F(UuidManagerTests, GetCanonicalUuid)
    {
        // Test that a legacy UUID is correctly upgraded to the canonical UUID
        using namespace AssetProcessor;

        static constexpr AZ::IO::FixedMaxPath FileA = "c:/somepath/fileA.txt";

        MakeFile(FileA);

        auto legacyUuids = m_uuidInterface->GetLegacyUuids(SourceAssetReference(FileA));

        ASSERT_TRUE(legacyUuids);

        auto canonicalUuid = m_uuidInterface->GetCanonicalUuid(*legacyUuids.GetValue().begin());

        ASSERT_TRUE(canonicalUuid);
        EXPECT_EQ(m_uuidInterface->GetUuid(SourceAssetReference(FileA)).GetValue(), canonicalUuid.value());
    }
}
