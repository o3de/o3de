/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/FileTag/FileTag.h>
#include <AzFramework/FileTag/FileTagBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AZTestShared/Utils/Utils.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <fstream>

namespace UnitTest
{

    const char DummyFile[] = "dummy.txt";
    const char AnotherDummyFile[] = "Foo/Dummy.txt";

    const char DummyPattern[] = R"(^(.+)_([a-z]+)\..+$)";
    const char MatchingPatternFile[] = "Foo/dummy_abc.txt";
    const char NonMatchingPatternFile[] = "Foo/dummy_a8c.txt";

    const char DummyWildcard[] = "?oo.txt";
    const char MatchingWildcardFile[] = "Foo.txt";
    const char NonMatchingWildcardFile[] = "Test.txt";

    const char* DummyFileTags[] = { "A", "B", "C", "D", "E", "F", "G" };
    const char* DummyFileTagsLowerCase[] = { "a", "b", "c", "d", "e", "f", "g" };

    enum DummyFileTagIndex
    {
        AIdx = 0,
        BIdx,
        CIdx,
        DIdx,
        EIdx,
        FIdx,
        GIdx
    };

    const char ExcludeFile[] = "Exclude";
    const char IncludeFile[] = "Include";

    class FileTagQueryManagerTest : public AzFramework::FileTag::FileTagQueryManager
    {
    public:
        friend class GTEST_TEST_CLASS_NAME_(FileTagTest, FileTags_QueryFilePlusPatternMatch_Valid);
        friend class GTEST_TEST_CLASS_NAME_(FileTagTest, FileTags_RemoveTag_Valid);

        FileTagQueryManagerTest(AzFramework::FileTag::FileTagType fileTagType)
            :AzFramework::FileTag::FileTagQueryManager(fileTagType)
        {
        }

        void ClearData()
        {
            m_fileTagsMap.clear();
            m_patternTagsMap.clear();
        }
    };

    class FileTagTest
        : public LeakDetectionFixture
    {
    public:

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_data = AZStd::make_unique<StaticData>();
            using namespace  AzFramework::FileTag;
            AZ::ComponentApplication::Descriptor desc;
            m_data->m_application.Start(desc);

            const char* testAssetRoot = m_tempDirectory.GetDirectory();

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            m_data->m_localFileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
            m_data->m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_data->m_localFileIO.get());

            AZ::IO::FileIOBase::GetInstance()->SetAlias("@products@", testAssetRoot);

            m_data->m_excludeFileQueryManager = AZStd::make_unique<FileTagQueryManagerTest>(FileTagType::Exclude);
            m_data->m_includeFileQueryManager = AZStd::make_unique<FileTagQueryManagerTest>(FileTagType::Include);

            AZStd::vector<AZStd::string> excludedFileTags = { DummyFileTags[DummyFileTagIndex::AIdx], DummyFileTags[DummyFileTagIndex::BIdx] };
            EXPECT_TRUE(m_data->m_fileTagManager.AddFileTags(DummyFile, FileTagType::Exclude, excludedFileTags).IsSuccess());

            AZStd::vector<AZStd::string> includedFileTags = { DummyFileTags[DummyFileTagIndex::CIdx], DummyFileTags[DummyFileTagIndex::DIdx] };
            EXPECT_TRUE(m_data->m_fileTagManager.AddFileTags(AnotherDummyFile, FileTagType::Include, includedFileTags).IsSuccess());

            AZStd::vector<AZStd::string> excludedPatternTags = { DummyFileTags[DummyFileTagIndex::EIdx], DummyFileTags[DummyFileTagIndex::FIdx] };
            EXPECT_TRUE(m_data->m_fileTagManager.AddFilePatternTags(DummyPattern, FilePatternType::Regex, FileTagType::Exclude, excludedPatternTags).IsSuccess());

            AZStd::vector<AZStd::string> includedWildcardTags = { DummyFileTags[DummyFileTagIndex::GIdx] };
            EXPECT_TRUE(m_data->m_fileTagManager.AddFilePatternTags(DummyWildcard, FilePatternType::Wildcard, FileTagType::Include, includedWildcardTags).IsSuccess());

            AzFramework::StringFunc::Path::Join(testAssetRoot, AZStd::string::format("%s.%s", ExcludeFile, FileTagAsset::Extension()).c_str(), m_data->m_excludeFile);

            AzFramework::StringFunc::Path::Join(testAssetRoot, AZStd::string::format("%s.%s", IncludeFile, FileTagAsset::Extension()).c_str(), m_data->m_includeFile);

            EXPECT_TRUE(m_data->m_fileTagManager.Save(FileTagType::Exclude, m_data->m_excludeFile));
            EXPECT_TRUE(m_data->m_fileTagManager.Save(FileTagType::Include, m_data->m_includeFile));

            EXPECT_TRUE(m_data->m_excludeFileQueryManager->Load(m_data->m_excludeFile));
            EXPECT_TRUE(m_data->m_includeFileQueryManager->Load(m_data->m_includeFile));

            AzFramework::StringFunc::Path::Join(testAssetRoot, "test_dependencies.xml", m_data->m_engineDependenciesFile);
            std::ofstream outFile(m_data->m_engineDependenciesFile.c_str(), std::ofstream::out | std::ofstream::app);
            outFile << "<EngineDependencies versionnumber=\"1.0.0\"><Dependency path=\"Foo\\Dummy.txt\" optional=\"true\"/><Dependency path=\"Foo/dummy_abc.txt\" optional=\"false\"/></EngineDependencies>";
            outFile.close();
        }

        void TearDown() override
        {

            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_data->m_priorFileIO);

            m_data->m_application.Stop();
            m_data.reset();

            LeakDetectionFixture::TearDown();
        }


        struct StaticData
        {
            AzFramework::Application m_application;
            AzFramework::FileTag::FileTagManager m_fileTagManager;
            AZStd::unique_ptr<FileTagQueryManagerTest> m_excludeFileQueryManager;
            AZStd::unique_ptr<FileTagQueryManagerTest> m_includeFileQueryManager;
            AZ::IO::FileIOBase* m_priorFileIO = nullptr;
            AZStd::unique_ptr<AZ::IO::FileIOBase> m_localFileIO;
            AZStd::string m_excludeFile;
            AZStd::string m_includeFile;
            AZStd::string m_engineDependenciesFile;
        };

        AZStd::unique_ptr<StaticData>       m_data;
        AZ::Test::ScopedAutoTempDirectory   m_tempDirectory;
    };

    TEST_F(FileTagTest, FileTags_QueryFile_Valid)
    {
        AZStd::set<AZStd::string> tags = m_data->m_excludeFileQueryManager->GetTags(DummyFile);

        ASSERT_EQ(tags.size(), 2);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::AIdx]), 1);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::BIdx]), 1);

        tags = m_data->m_includeFileQueryManager->GetTags(DummyFile);
        ASSERT_EQ(tags.size(), 0);

        tags = m_data->m_includeFileQueryManager->GetTags(AnotherDummyFile);
        ASSERT_EQ(tags.size(), 2);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::CIdx]), 1);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::DIdx]), 1);

        tags = m_data->m_excludeFileQueryManager->GetTags(AnotherDummyFile);
        ASSERT_EQ(tags.size(), 0);
    }

    TEST_F(FileTagTest, FileTags_QueryByAbsoluteFilePath_Valid)
    {
        AZStd::string absoluteDummyFilePath = DummyFile;
        EXPECT_TRUE(AzFramework::StringFunc::AssetDatabasePath::Join("@products@", absoluteDummyFilePath.c_str(), absoluteDummyFilePath));

        AZStd::set<AZStd::string> tags = m_data->m_excludeFileQueryManager->GetTags(absoluteDummyFilePath);

        ASSERT_EQ(tags.size(), 2);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::AIdx]), 1);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::BIdx]), 1);

        tags = m_data->m_includeFileQueryManager->GetTags(absoluteDummyFilePath);
        ASSERT_EQ(tags.size(), 0);

        AZStd::string absoluteAnotherDummyFilePath = AnotherDummyFile;
        EXPECT_TRUE(AzFramework::StringFunc::AssetDatabasePath::Join("@products@", absoluteAnotherDummyFilePath.c_str(), absoluteAnotherDummyFilePath));

        tags = m_data->m_includeFileQueryManager->GetTags(absoluteAnotherDummyFilePath);
        ASSERT_EQ(tags.size(), 2);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::CIdx]), 1);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::DIdx]), 1);

        tags = m_data->m_excludeFileQueryManager->GetTags(absoluteAnotherDummyFilePath);
        ASSERT_EQ(tags.size(), 0);
    }

    TEST_F(FileTagTest, FileTags_QueryTagsDefinedForFilePathWithAlias_Valid)
    {
        using namespace  AzFramework::FileTag;

        // Set the customized alias
        AZStd::string customizedAliasFilePath;
        const char* assetsAlias = AZ::IO::FileIOBase::GetInstance()->GetAlias("@products@");
        AzFramework::StringFunc::AssetDatabasePath::Join(assetsAlias, "foo", customizedAliasFilePath);
        AZ::IO::FileIOBase::GetInstance()->SetAlias("@customizedalias@", customizedAliasFilePath.c_str());

        // Add tags for a file path with this customzied alias
        AZStd::string DummyFileWithcustomizedAlias;
        EXPECT_TRUE(AzFramework::StringFunc::AssetDatabasePath::Join("@customizedalias@", "dummy.txt", DummyFileWithcustomizedAlias));

        AZStd::vector<AZStd::string> excludedFileTags = { DummyFileTags[DummyFileTagIndex::CIdx], DummyFileTags[DummyFileTagIndex::DIdx] };
        EXPECT_TRUE(m_data->m_fileTagManager.AddFileTags(DummyFileWithcustomizedAlias, FileTagType::Exclude, excludedFileTags).IsSuccess());

        EXPECT_TRUE(m_data->m_fileTagManager.Save(FileTagType::Exclude, m_data->m_excludeFile));

        m_data->m_excludeFileQueryManager->ClearData();
        EXPECT_TRUE(m_data->m_excludeFileQueryManager->Load(m_data->m_excludeFile));

        // Query the file and verify the tags were added successfully
        AZStd::set<AZStd::string> tags = m_data->m_excludeFileQueryManager->GetTags(AnotherDummyFile);

        ASSERT_EQ(tags.size(), 2);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::CIdx]), 1);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::DIdx]), 1);
    }

    TEST_F(FileTagTest, FileTags_QueryPattern_Valid)
    {
        AZStd::set<AZStd::string> tags = m_data->m_excludeFileQueryManager->GetTags(MatchingPatternFile);

        ASSERT_EQ(tags.size(), 2);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::EIdx]), 1);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::FIdx]), 1);

        tags = m_data->m_includeFileQueryManager->GetTags(MatchingPatternFile);
        ASSERT_EQ(tags.size(), 0);

        tags = m_data->m_excludeFileQueryManager->GetTags(NonMatchingPatternFile);
        ASSERT_EQ(tags.size(), 0);

        tags = m_data->m_includeFileQueryManager->GetTags(NonMatchingPatternFile);
        ASSERT_EQ(tags.size(), 0);
    }

    TEST_F(FileTagTest, FileTags_QueryWildcard_Valid)
    {
        AZStd::set<AZStd::string> tags = m_data->m_includeFileQueryManager->GetTags(MatchingWildcardFile);

        ASSERT_EQ(tags.size(), 1);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::GIdx]), 1);

        tags = m_data->m_excludeFileQueryManager->GetTags(MatchingWildcardFile);
        ASSERT_EQ(tags.size(), 0);

        tags = m_data->m_excludeFileQueryManager->GetTags(NonMatchingWildcardFile);
        ASSERT_EQ(tags.size(), 0);

        tags = m_data->m_includeFileQueryManager->GetTags(NonMatchingWildcardFile);
        ASSERT_EQ(tags.size(), 0);
    }

    TEST_F(FileTagTest, FileTags_LoadEngineDependencies_AddToExcludeFile)
    {
        m_data->m_excludeFileQueryManager->ClearData();
        EXPECT_TRUE(m_data->m_excludeFileQueryManager->LoadEngineDependencies(m_data->m_engineDependenciesFile));

        AZStd::string normalizedFilePath = AnotherDummyFile;
        EXPECT_TRUE(AzFramework::StringFunc::Path::Normalize(normalizedFilePath));

        AZStd::set<AZStd::string> outputTags = m_data->m_excludeFileQueryManager->GetTags(normalizedFilePath);
        EXPECT_EQ(outputTags.size(), 2);
        EXPECT_EQ(outputTags.count("ignore"), 1);
        EXPECT_EQ(outputTags.count("productdependency"), 1);

        normalizedFilePath = MatchingPatternFile;
        EXPECT_TRUE(AzFramework::StringFunc::Path::Normalize(normalizedFilePath));

        outputTags = m_data->m_excludeFileQueryManager->GetTags(normalizedFilePath);
        EXPECT_EQ(outputTags.size(), 0);
    }

    TEST_F(FileTagTest, FileTags_QueryFilePlusPatternMatch_Valid)
    {
        using namespace  AzFramework::FileTag;
        AZStd::vector<AZStd::string>  inputTags = { DummyFileTags[DummyFileTagIndex::GIdx] };
        EXPECT_TRUE(m_data->m_fileTagManager.AddFileTags(MatchingWildcardFile, FileTagType::Exclude, inputTags).IsSuccess());

        inputTags = { DummyFileTags[DummyFileTagIndex::AIdx] };
        EXPECT_TRUE(m_data->m_fileTagManager.AddFilePatternTags(DummyWildcard, FilePatternType::Wildcard, FileTagType::Exclude, inputTags).IsSuccess());

        EXPECT_TRUE(m_data->m_fileTagManager.Save(FileTagType::Exclude, m_data->m_excludeFile));

        m_data->m_excludeFileQueryManager->ClearData();
        EXPECT_TRUE(m_data->m_excludeFileQueryManager->Load(m_data->m_excludeFile));

        AZStd::set<AZStd::string> outputTags = m_data->m_excludeFileQueryManager->GetTags(MatchingWildcardFile);

        EXPECT_EQ(outputTags.size(), 2);
        EXPECT_EQ(outputTags.count(DummyFileTagsLowerCase[DummyFileTagIndex::AIdx]), 1);
        EXPECT_EQ(outputTags.count(DummyFileTagsLowerCase[DummyFileTagIndex::GIdx]), 1);
    }

    TEST_F(FileTagTest, FileTags_RemoveTag_Valid)
    {
        using namespace  AzFramework::FileTag;
        AZStd::set<AZStd::string> tags = m_data->m_excludeFileQueryManager->GetTags(DummyFile);

        ASSERT_EQ(tags.size(), 2);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::AIdx]), 1);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::BIdx]), 1);

        //remove Tag A
        AZStd::vector<AZStd::string> excludedFileTags = { DummyFileTags[DummyFileTagIndex::AIdx] };
        EXPECT_TRUE(m_data->m_fileTagManager.RemoveFileTags(DummyFile, FileTagType::Exclude, excludedFileTags).IsSuccess());

        EXPECT_TRUE(m_data->m_fileTagManager.Save(FileTagType::Exclude, m_data->m_excludeFile));

        m_data->m_excludeFileQueryManager->ClearData();
        EXPECT_TRUE(m_data->m_excludeFileQueryManager->Load(m_data->m_excludeFile));

        tags = m_data->m_excludeFileQueryManager->GetTags(DummyFile);

        ASSERT_EQ(tags.size(), 1);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::BIdx]), 1);

        //remove Tag B
        excludedFileTags = { DummyFileTags[DummyFileTagIndex::BIdx] };
        EXPECT_TRUE(m_data->m_fileTagManager.RemoveFileTags(DummyFile, FileTagType::Exclude, excludedFileTags).IsSuccess());

        EXPECT_TRUE(m_data->m_fileTagManager.Save(FileTagType::Exclude, m_data->m_excludeFile));

        m_data->m_excludeFileQueryManager->ClearData();
        EXPECT_TRUE(m_data->m_excludeFileQueryManager->Load(m_data->m_excludeFile));

        tags = m_data->m_excludeFileQueryManager->GetTags(DummyFile);

        ASSERT_EQ(tags.size(), 0);
    }

    TEST_F(FileTagTest, FileTags_Matching_Valid)
    {
        AZStd::vector<AZStd::string> matchFileTags = { DummyFileTags[DummyFileTagIndex::AIdx], DummyFileTags[DummyFileTagIndex::BIdx] };

        bool match = m_data->m_excludeFileQueryManager->Match(DummyFile, matchFileTags);
        EXPECT_TRUE(match);

        matchFileTags = { DummyFileTags[DummyFileTagIndex::AIdx] };
        match = m_data->m_excludeFileQueryManager->Match(DummyFile, matchFileTags);
        EXPECT_TRUE(match);

        matchFileTags = { DummyFileTags[DummyFileTagIndex::BIdx] };
        match = m_data->m_excludeFileQueryManager->Match(DummyFile, matchFileTags);
        EXPECT_TRUE(match);

        matchFileTags = { DummyFileTags[DummyFileTagIndex::CIdx] };
        match = m_data->m_excludeFileQueryManager->Match(DummyFile, matchFileTags);
        EXPECT_FALSE(match);
    }

    TEST_F(FileTagTest, FileTags_ValidateError_Ok)
    {
        using namespace  AzFramework::FileTag;
        AZStd::set<AZStd::string> tags = m_data->m_excludeFileQueryManager->GetTags(DummyFile);

        ASSERT_EQ(tags.size(), 2);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::AIdx]), 1);
        EXPECT_EQ(tags.count(DummyFileTagsLowerCase[DummyFileTagIndex::BIdx]), 1);

        // file tags already exists
        AZStd::vector<AZStd::string> excludedFileTags = { DummyFileTags[DummyFileTagIndex::AIdx], DummyFileTags[DummyFileTagIndex::BIdx] };

        EXPECT_TRUE(m_data->m_fileTagManager.AddFileTags(DummyFile, FileTagType::Exclude, excludedFileTags).IsSuccess());

        //remove Tag C which does not exist
        excludedFileTags = { DummyFileTags[DummyFileTagIndex::CIdx] };
        EXPECT_TRUE(m_data->m_fileTagManager.RemoveFileTags(DummyFile, FileTagType::Exclude, excludedFileTags).IsSuccess());
        // Invalid FilePattern type
        AZStd::vector<AZStd::string> excludedPatternTags = { DummyFileTags[DummyFileTagIndex::EIdx], DummyFileTags[DummyFileTagIndex::FIdx] };
        EXPECT_FALSE(m_data->m_fileTagManager.RemoveFilePatternTags(DummyPattern, FilePatternType::Wildcard, FileTagType::Exclude, excludedPatternTags).IsSuccess());

        // Removing a FilePattern that does not exits
        const char pattern[] = R"(^(.+)_([a-z0-9]+)\..+$)";
        excludedPatternTags = { DummyFileTags[DummyFileTagIndex::EIdx], DummyFileTags[DummyFileTagIndex::FIdx] };
        EXPECT_FALSE(m_data->m_fileTagManager.RemoveFilePatternTags(pattern, FilePatternType::Regex, FileTagType::Exclude, excludedPatternTags).IsSuccess());
    }
}
