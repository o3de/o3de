/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QHash>

#include "native/tests/AssetProcessorTest.h"
#include <native/utilities/PlatformConfiguration.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/parallel/thread.h>
#include <AzTest/AzTest.h>
#include <tests/UnitTestUtilities.h>


using namespace AssetUtilities;

class AssetUtilitiesTest
    : public AssetProcessor::AssetProcessorTest
{
    void SetUp() override
    {
        AssetProcessorTest::SetUp();

        if (AZ::IO::FileIOBase::GetInstance() == nullptr)
        {
            m_localFileIo = aznew AZ::IO::LocalFileIO();
            AZ::IO::FileIOBase::SetInstance(m_localFileIo);
        }
    }

    void TearDown() override
    {
        if (m_localFileIo)
        {
            delete m_localFileIo;
            m_localFileIo = nullptr;
            AZ::IO::FileIOBase::SetInstance(nullptr);
        }

        AssetProcessorTest::TearDown();
    }

    AZ::IO::FileIOBase* m_localFileIo{};
};

TEST_F(AssetUtilitiesTest, NormlizeFilePath_NormalizedValidPathRelPath_Valid)
{
    QString result = NormalizeFilePath("a/b\\c\\d/E.txt");
    EXPECT_STREQ(result.toUtf8().constData(), "a/b/c/d/E.txt");
}

TEST_F(AssetUtilitiesTest, NormlizeFilePath_NormalizedValidPathFullPath_Valid)
{
    QString result = NormalizeFilePath("c:\\a/b\\c\\d/E.txt");
    // on windows, drive letters are normalized to full
#if defined(AZ_PLATFORM_WINDOWS)
    ASSERT_TRUE(result.compare("C:/a/b/c/d/E.txt", Qt::CaseSensitive) == 0);
#else
    // on other platforms, C: is a relative path to a file called 'c:')
    EXPECT_STREQ(result.toUtf8().constData(), "c:/a/b/c/d/E.txt");
#endif
}

TEST_F(AssetUtilitiesTest, NormlizeFilePath_NormalizedValidDirRelPath_Valid)
{
    QString result = NormalizeDirectoryPath("a/b\\c\\D");
    EXPECT_STREQ(result.toUtf8().constData(), "a/b/c/D");
}

TEST_F(AssetUtilitiesTest, NormlizeFilePath_NormalizedValidDirFullPath_Valid)
{
    QString result = NormalizeDirectoryPath("c:\\a/b\\C\\d\\");

    // on windows, drive letters are normalized to full
#if defined(AZ_PLATFORM_WINDOWS)
    EXPECT_STREQ(result.toUtf8().constData(), "C:/a/b/C/d");
#else
    EXPECT_STREQ(result.toUtf8().constData(), "c:/a/b/C/d");
#endif
}

TEST_F(AssetUtilitiesTest, ComputeCRC32Lowercase_IsCaseInsensitive)
{
    const char* upperCaseString = "HELLOworld";
    const char* lowerCaseString = "helloworld";

    EXPECT_EQ(AssetUtilities::ComputeCRC32Lowercase(lowerCaseString), AssetUtilities::ComputeCRC32Lowercase(upperCaseString));

    // also try the length-based one.
    EXPECT_EQ(AssetUtilities::ComputeCRC32Lowercase(lowerCaseString, size_t(5)), AssetUtilities::ComputeCRC32Lowercase(upperCaseString, size_t(5)));
}

TEST_F(AssetUtilitiesTest, ComputeCRC32_IsCaseSensitive)
{
    const char* upperCaseString = "HELLOworld";
    const char* lowerCaseString = "helloworld";

    EXPECT_NE(AssetUtilities::ComputeCRC32(lowerCaseString), AssetUtilities::ComputeCRC32(upperCaseString));

    // also try the length-based one.
    EXPECT_NE(AssetUtilities::ComputeCRC32(lowerCaseString, size_t(5)), AssetUtilities::ComputeCRC32(upperCaseString, size_t(5)));
}


TEST_F(AssetUtilitiesTest, UpdateToCorrectCase_MissingFile_ReturnsFalse)
{
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);

    QString fileName = "someFile.txt";
    EXPECT_FALSE(AssetUtilities::UpdateToCorrectCase(canonicalTempDirPath, fileName));
}

TEST_F(AssetUtilitiesTest, UpdateToCorrectCase_ExistingFile_ReturnsTrue_CorrectsCase)
{
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);

    QStringList thingsToTry;
    thingsToTry << "SomeFile.TxT";
    thingsToTry << "otherfile.txt";
    thingsToTry << "subfolder1/otherfile.txt";

    #if defined(AZ_PLATFORM_WINDOWS)
    thingsToTry << "subfolder2\\otherfile.txt";
    thingsToTry << "subFolder3\\somefile.txt";
    thingsToTry << "subFolder4\\subfolder6\\somefile.txt";
    thingsToTry << "subFolder5\\subfolder7/someFile.txt";
    #endif // AZ_PLATFORM_WINDOWS
    thingsToTry << "specialFileName[.txt";
    thingsToTry << "specialFileName].txt";
    thingsToTry << "specialFileName!.txt";
    thingsToTry << "specialFileName#.txt";
    thingsToTry << "specialFileName$.txt";
    thingsToTry << "specialFile%Name%.txt";
    thingsToTry << "specialFileName&.txt";
    thingsToTry << "specialFileName(.txt";
    thingsToTry << "specialFileName+.txt";
    thingsToTry << "specialFileName[9].txt";
    thingsToTry << "specialFileName[A-Za-z].txt"; // these should all be treated as literally the name of the file, not a regex!

    for (QString triedThing : thingsToTry)
    {
        triedThing = NormalizeFilePath(triedThing);
        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath(triedThing)));

        QString lowercaseVersion = triedThing.toLower();
        // each one should be found.   If it fails, we'll pipe out the name of the file it fails on for extra context.

        EXPECT_TRUE(AssetUtilities::UpdateToCorrectCase(canonicalTempDirPath, lowercaseVersion)) << "File being Examined: " << lowercaseVersion.toUtf8().constData();
        // each one should correct, and return a normalized path.
        EXPECT_STREQ(AssetUtilities::NormalizeFilePath(lowercaseVersion).toUtf8().constData(), AssetUtilities::NormalizeFilePath(triedThing).toUtf8().constData());
    }
}

TEST_F(AssetUtilitiesTest, GenerateFingerprint_BasicTest)
{
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);
    QString absoluteTestFilePath1 = tempPath.absoluteFilePath("basicfile.txt");
    QString absoluteTestFilePath2 = tempPath.absoluteFilePath("basicfile2.txt");
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath1, "contents"));
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath2, "contents"));

    AZStd::string fileEncoded1 = absoluteTestFilePath1.toUtf8().constData();
    AZStd::string fileEncoded2 = absoluteTestFilePath2.toUtf8().constData();

    AssetProcessor::JobDetails jobDetail;
    // it is expected that the only parts of jobDetails that matter are:
    // jobDetail.m_extraInformationForFingerprinting
    // jobDetail.m_fingerprintFiles
    // jobDetail.m_jobDependencyList

    jobDetail.m_extraInformationForFingerprinting = "extra info1";
    // the fingerprint should always be stable over repeated runs, even with minimal info:
    unsigned int result1 = AssetUtilities::GenerateFingerprint(jobDetail);
    unsigned int result2 = AssetUtilities::GenerateFingerprint(jobDetail);
    EXPECT_EQ(result1, result2);

    // the fingerprint should always be different when anything changes:
    jobDetail.m_extraInformationForFingerprinting = "extra info1";
    result1 = AssetUtilities::GenerateFingerprint(jobDetail);
    jobDetail.m_extraInformationForFingerprinting = "extra info2";
    result2 = AssetUtilities::GenerateFingerprint(jobDetail);
    EXPECT_NE(result1, result2);

    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(absoluteTestFilePath1.toUtf8().constData(), "basicfile.txt"));
    result1 = AssetUtilities::GenerateFingerprint(jobDetail);
    result2 = AssetUtilities::GenerateFingerprint(jobDetail);
    EXPECT_EQ(result1, result2);

    // mutating the dependency list should mutate the fingerprint, even if the extra info doesn't change.
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(absoluteTestFilePath2.toUtf8().constData(), "basicfile2.txt"));
    result2 = AssetUtilities::GenerateFingerprint(jobDetail);
    EXPECT_NE(result1, result2);

    UnitTestUtils::SleepForMinimumFileSystemTime();

    // mutating the actual files should mutate the fingerprint, even if the file list doesn't change.
    // note that both files are in the file list, so changing just the one should result in a change in hash:
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath1, "contents new"));
    result1 = AssetUtilities::GenerateFingerprint(jobDetail);
    EXPECT_NE(result1, result2);

    // changing the other should also change the hash:
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath2, "contents new2"));
    result2 = AssetUtilities::GenerateFingerprint(jobDetail);
    EXPECT_NE(result1, result2);
}

TEST_F(AssetUtilitiesTest, GenerateFingerprint_Empty_Asserts)
{
    AssetProcessor::JobDetails jobDetail;
    AssetUtilities::GenerateFingerprint(jobDetail);

    EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 1);
    m_errorAbsorber->Clear();
}

TEST_F(AssetUtilitiesTest, GenerateFingerprint_MissingFile_NotSameAsZeroByteFile)
{
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);
    QString absoluteTestFilePath1 = tempPath.absoluteFilePath("basicfile.txt");
    QString absoluteTestFilePath2 = tempPath.absoluteFilePath("basicfile2.txt");

    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath1, "")); // empty file
    // note:  basicfile1 exists but is empty, whereas basicfile2, 3 are missing entirely.

    AssetProcessor::JobDetails jobDetail;
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile.txt").toUtf8().constData(), "basicfile.txt"));
    AZ::u32 fingerprint1 = AssetUtilities::GenerateFingerprint(jobDetail);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile2.txt").toUtf8().constData(), "basicfile2.txt"));
    AZ::u32 fingerprint2 = AssetUtilities::GenerateFingerprint(jobDetail);

    EXPECT_NE(fingerprint1, fingerprint2);
}

TEST_F(AssetUtilitiesTest, GenerateFingerprint_MissingFile_NotSameAsOtherMissingFile)
{
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);
    QString absoluteTestFilePath1 = tempPath.absoluteFilePath("basicfile.txt");
    QString absoluteTestFilePath2 = tempPath.absoluteFilePath("basicfile2.txt");

    // we create no files on disk.

    AssetProcessor::JobDetails jobDetail;
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile.txt").toUtf8().constData(), "basicfile.txt"));
    AZ::u32 fingerprint1 = AssetUtilities::GenerateFingerprint(jobDetail);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile2.txt").toUtf8().constData(), "basicfile2.txt"));
    AZ::u32 fingerprint2 = AssetUtilities::GenerateFingerprint(jobDetail);

    EXPECT_NE(fingerprint1, fingerprint2);
}

TEST_F(AssetUtilitiesTest, GenerateFingerprint_OneFile_Differs)
{
    // this test makes sure that changing each part of jobDetail relevant to fingerprints causes the resulting fingerprint to change.
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);
    QString absoluteTestFilePath1 = tempPath.absoluteFilePath("basicfile.txt");
    QString absoluteTestFilePath2 = tempPath.absoluteFilePath("basicfile2.txt");
    QString absoluteTestFilePath3 = tempPath.absoluteFilePath("basicfile3.txt");

    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath1, "contents"));
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath2, "contents")); // same contents
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath3, "contents2")); // different contents

    AssetProcessor::JobDetails jobDetail;
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile.txt").toUtf8().constData(), "basicfile.txt"));

    AZ::u32 fingerprint1 = AssetUtilities::GenerateFingerprint(jobDetail);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile2.txt").toUtf8().constData(), "basicfile2.txt"));
    AZ::u32 fingerprint2 = AssetUtilities::GenerateFingerprint(jobDetail);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile3.txt").toUtf8().constData(), "basicfile3.txt"));
    AZ::u32 fingerprint3 = AssetUtilities::GenerateFingerprint(jobDetail);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile.txt").toUtf8().constData(), "basicfile.txt"));
    EXPECT_EQ(AssetUtilities::GenerateFingerprint(jobDetail), fingerprint1);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile2.txt").toUtf8().constData(), "basicfile2.txt"));
    EXPECT_EQ(AssetUtilities::GenerateFingerprint(jobDetail), fingerprint2);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile3.txt").toUtf8().constData(), "basicfile3.txt"));
    EXPECT_EQ(AssetUtilities::GenerateFingerprint(jobDetail), fingerprint3);

    EXPECT_NE(fingerprint1, fingerprint2);
    EXPECT_NE(fingerprint2, fingerprint3);
    EXPECT_NE(fingerprint3, fingerprint1);
}

TEST_F(AssetUtilitiesTest, GenerateFingerprint_MultipleFile_Differs)
{
    // given multiple files, make sure that the fingerprint for multiple files differs from the one file (that each file is taken into account)
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);
    QString absoluteTestFilePath1 = tempPath.absoluteFilePath("basicfile.txt");
    QString absoluteTestFilePath2 = tempPath.absoluteFilePath("basicfile2.txt");
    QString absoluteTestFilePath3 = tempPath.absoluteFilePath("basicfile3.txt");

    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath1, "contents"));
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath2, "contents")); // same contents
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath3, "contents2")); // different contents

    AssetProcessor::JobDetails jobDetail;
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile.txt").toUtf8().constData(), "basicfile.txt"));
    AZ::u32 fingerprint1 = AssetUtilities::GenerateFingerprint(jobDetail);
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile2.txt").toUtf8().constData(), "basicfile2.txt"));
    AZ::u32 fingerprint2 = AssetUtilities::GenerateFingerprint(jobDetail);
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile3.txt").toUtf8().constData(), "basicfile3.txt"));
    AZ::u32 fingerprint3 = AssetUtilities::GenerateFingerprint(jobDetail);

    EXPECT_NE(fingerprint1, fingerprint2);
    EXPECT_NE(fingerprint2, fingerprint3);
    EXPECT_NE(fingerprint3, fingerprint1);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile.txt").toUtf8().constData(), "basicfile.txt"));
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile2.txt").toUtf8().constData(), "basicfile2.txt"));
    fingerprint1 = AssetUtilities::GenerateFingerprint(jobDetail);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile2.txt").toUtf8().constData(), "basicfile2.txt"));
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile3.txt").toUtf8().constData(), "basicfile3.txt"));
    fingerprint2 = AssetUtilities::GenerateFingerprint(jobDetail);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile1.txt").toUtf8().constData(), "basicfile1.txt"));
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile3.txt").toUtf8().constData(), "basicfile3.txt"));
    fingerprint3 = AssetUtilities::GenerateFingerprint(jobDetail);

    EXPECT_NE(fingerprint1, fingerprint2);
    EXPECT_NE(fingerprint2, fingerprint3);
    EXPECT_NE(fingerprint3, fingerprint1);
}

TEST_F(AssetUtilitiesTest, GenerateFingerprint_OrderOnceJobDependency_NoChange)
{
    // OrderOnce Job dependency should not alter the fingerprint of the job
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    UnitTests::MockPathConversion mockPathConversion(dir.path().toUtf8().constData());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);
    const char relFile1Path[] = "file.txt";
    const char relFile2Path[] = "secondFile.txt";
    QString absoluteTestFile1Path = tempPath.absoluteFilePath(relFile1Path);
    QString absoluteTestFile2Path = tempPath.absoluteFilePath(relFile2Path);

    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFile1Path, "contents"));
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFile2Path, "contents"));

    AssetProcessor::JobDetails jobDetail;

    jobDetail.m_jobEntry.m_sourceAssetReference = AssetProcessor::SourceAssetReference(tempPath.absolutePath(), relFile1Path);
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(absoluteTestFile1Path.toUtf8().constData(), relFile1Path));

    AZ::u32 fingerprintWithoutOrderOnceJobDependency = AssetUtilities::GenerateFingerprint(jobDetail);

    AssetBuilderSDK::SourceFileDependency dep = { relFile2Path, AZ::Uuid::CreateNull() };
    AssetBuilderSDK::JobDependency jobDep("key", "pc", AssetBuilderSDK::JobDependencyType::OrderOnce, dep);
    jobDetail.m_jobDependencyList.push_back(AssetProcessor::JobDependencyInternal(jobDep));

    AZ::u32 fingerprintWithOrderOnceJobDependency = AssetUtilities::GenerateFingerprint(jobDetail);

    EXPECT_EQ(fingerprintWithoutOrderOnceJobDependency, fingerprintWithOrderOnceJobDependency);
}

TEST_F(AssetUtilitiesTest, GenerateFingerprint_OrderOnlyJobDependency_NoChange)
{
    // OrderOnly Job dependency should not alter the fingerprint of the job
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    UnitTests::MockPathConversion mockPathConversion(dir.path().toUtf8().constData());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);
    const char relFile1Path[] = "file.txt";
    const char relFile2Path[] = "secondFile.txt";
    QString absoluteTestFile1Path = tempPath.absoluteFilePath(relFile1Path);
    QString absoluteTestFile2Path = tempPath.absoluteFilePath(relFile2Path);

    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFile1Path, "contents"));
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFile2Path, "contents"));

    AssetProcessor::JobDetails jobDetail;

    jobDetail.m_jobEntry.m_sourceAssetReference = AssetProcessor::SourceAssetReference(tempPath.absolutePath(), relFile1Path);
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(absoluteTestFile1Path.toUtf8().constData(), relFile1Path));

    AZ::u32 fingerprintWithoutOrderOnlyJobDependency = AssetUtilities::GenerateFingerprint(jobDetail);

    AssetBuilderSDK::SourceFileDependency dep = { relFile2Path, AZ::Uuid::CreateNull() };
    AssetBuilderSDK::JobDependency jobDep("key", "pc", AssetBuilderSDK::JobDependencyType::OrderOnly, dep);
    jobDetail.m_jobDependencyList.push_back(AssetProcessor::JobDependencyInternal(jobDep));

    AZ::u32 fingerprintWithOrderOnlyJobDependency = AssetUtilities::GenerateFingerprint(jobDetail);

    EXPECT_EQ(fingerprintWithoutOrderOnlyJobDependency, fingerprintWithOrderOnlyJobDependency);
}

namespace AssetUtilsTest
{
    class MockJobDependencyResponder : public AssetProcessor::ProcessingJobInfoBus::Handler
    {
    public:
        MOCK_METHOD1(GetJobFingerprint, AZ::u32(const AssetProcessor::JobIndentifier&));

        ~MockJobDependencyResponder()
        {
            BusDisconnect();
        }
    };
}

TEST_F(AssetUtilitiesTest, GenerateFingerprint_GivenJobDependencies_AffectsOutcome)
{
    using namespace testing;
    using ::testing::NiceMock;

    NiceMock<AssetUtilsTest::MockJobDependencyResponder> responder;

    responder.BusConnect();

    QTemporaryDir dir;
    QDir tempPath(dir.path());
    UnitTests::MockPathConversion mockPathConversion(dir.path().toUtf8().constData());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);
    QString absoluteTestFilePath1 = tempPath.absoluteFilePath("basicfile.txt");

    AssetProcessor::JobDetails jobDetail;
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile.txt").toUtf8().constData(), "basicfile.txt"));
    AZ::u32 fingerprint1 = AssetUtilities::GenerateFingerprint(jobDetail);

    // add a job dependency - it should alter the fingerprint, even if the file does not exist.
    AssetBuilderSDK::JobDependency jobDep("thing", "pc", AssetBuilderSDK::JobDependencyType::Order, AssetBuilderSDK::SourceFileDependency(tempPath.absoluteFilePath("basicfile.txt").toUtf8().constData(), AZ::Uuid::CreateNull()));
    AssetProcessor::JobDependencyInternal internalJobDep(jobDep);
    internalJobDep.m_builderUuidList.insert(AZ::Uuid::CreateRandom());
    jobDetail.m_jobDependencyList.push_back(internalJobDep);

    EXPECT_CALL(responder, GetJobFingerprint(_))
        .WillOnce(
            Return(0x12341234));

    AZ::u32 fingerprint2 = AssetUtilities::GenerateFingerprint(jobDetail);

    // different job fingerprint -> different result
    EXPECT_CALL(responder, GetJobFingerprint(_))
        .WillOnce(
            Return(0x11111111));

    AZ::u32 fingerprint3 = AssetUtilities::GenerateFingerprint(jobDetail);

    EXPECT_NE(fingerprint1, fingerprint2);
    EXPECT_NE(fingerprint2, fingerprint3);
    EXPECT_NE(fingerprint3, fingerprint1);
}

TEST_F(AssetUtilitiesTest, GetFileFingerprint_BasicTest)
{
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    UnitTests::MockPathConversion mockPathConversion(dir.path().toUtf8().constData());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);
    QString absoluteTestFilePath1 = tempPath.absoluteFilePath("basicfile.txt");
    QString absoluteTestFilePath2 = tempPath.absoluteFilePath("basicfile2.txt");

    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath1, "contents"));
    UnitTestUtils::SleepForMinimumFileSystemTime();
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath2, "contents2"));

    AZStd::string fileEncoded1 = absoluteTestFilePath1.toUtf8().constData();
    AZStd::string fileEncoded2 = absoluteTestFilePath2.toUtf8().constData();

    // repeatedly hashing the same file should result in the same hash:
    EXPECT_STREQ(AssetUtilities::GetFileFingerprint(fileEncoded1, "").c_str(), AssetUtilities::GetFileFingerprint(fileEncoded1, "").c_str());
    EXPECT_STREQ(AssetUtilities::GetFileFingerprint(fileEncoded1, "Name").c_str(), AssetUtilities::GetFileFingerprint(fileEncoded1, "Name").c_str());
    EXPECT_STREQ(AssetUtilities::GetFileFingerprint(fileEncoded2, "").c_str(), AssetUtilities::GetFileFingerprint(fileEncoded2, "").c_str());
    EXPECT_STREQ(AssetUtilities::GetFileFingerprint(fileEncoded2, "Name").c_str(), AssetUtilities::GetFileFingerprint(fileEncoded2, "Name").c_str());

    // mutating the 'name' should mutate the fingerprint:
    EXPECT_STRNE(AssetUtilities::GetFileFingerprint(fileEncoded1, "").c_str(), AssetUtilities::GetFileFingerprint(fileEncoded1, "Name").c_str());

    // two different files should not hash to the same fingerprint:
    EXPECT_STRNE(AssetUtilities::GetFileFingerprint(fileEncoded1, "").c_str(), AssetUtilities::GetFileFingerprint(fileEncoded2, "").c_str());

    UnitTestUtils::SleepForMinimumFileSystemTime();

    AZStd::string oldFingerprint1 = AssetUtilities::GetFileFingerprint(fileEncoded1, "");
    AZStd::string oldFingerprint2 = AssetUtilities::GetFileFingerprint(fileEncoded2, "");
    AZStd::string oldFingerprint1a = AssetUtilities::GetFileFingerprint(fileEncoded1, "Name1");
    AZStd::string oldFingerprint2a = AssetUtilities::GetFileFingerprint(fileEncoded2, "Name2");

    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath1, "contents1a"));
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath2, "contents2a"));

    EXPECT_STRNE(oldFingerprint1.c_str(), AssetUtilities::GetFileFingerprint(fileEncoded1, "").c_str());
    EXPECT_STRNE(oldFingerprint2.c_str(), AssetUtilities::GetFileFingerprint(fileEncoded2, "").c_str());
    EXPECT_STRNE(oldFingerprint1a.c_str(), AssetUtilities::GetFileFingerprint(fileEncoded1, "Name1").c_str());
    EXPECT_STRNE(oldFingerprint2a.c_str(), AssetUtilities::GetFileFingerprint(fileEncoded2, "Name2").c_str());
}


TEST_F(AssetUtilitiesTest, GetFileFingerprint_NonExistentFiles)
{
    AZStd::string nonExistentFile1 = AZ::Uuid::CreateRandom().ToString<AZStd::string>() + ".txt";
    ASSERT_FALSE(QFileInfo::exists(nonExistentFile1.c_str()));

    EXPECT_STRNE(AssetUtilities::GetFileFingerprint(nonExistentFile1, "").c_str(), AssetUtilities::GetFileFingerprint(nonExistentFile1, "Name").c_str());
    EXPECT_STREQ(AssetUtilities::GetFileFingerprint(nonExistentFile1, "Name").c_str(), AssetUtilities::GetFileFingerprint(nonExistentFile1, "Name").c_str());
}

TEST_F(AssetUtilitiesTest, CreateDirWithTimeout_Valid)
{
    QTemporaryDir tempDir;
    QDir tempPath(tempDir.path());
    QDir dir(tempPath.filePath("folder"));
    unsigned int timeToWaitInSecs = 3;
    AZStd::vector<AZStd::thread*> threadList;
    AZStd::vector<bool> resultList;
    AZStd::mutex resultMutex;
    auto runFunc = [&]()
    {
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));// sleeping to sync all the threads
        bool result = AssetUtilities::CreateDirectoryWithTimeout(dir, timeToWaitInSecs);
        AZStd::lock_guard<AZStd::mutex> locker(resultMutex);
        resultList.push_back(result);
    };

    int numberOfThreads = 5;

    ASSERT_FALSE(dir.exists());

    for (int idx = 0; idx < numberOfThreads; idx++)
    {
        AZStd::thread* workerThread = new AZStd::thread(runFunc);
        threadList.emplace_back(workerThread);
    }

    for (auto thread : threadList)
    {
        if (thread->joinable())
        {
            thread->join();
        }
        delete thread;
    }

    for (int idx = 0; idx < numberOfThreads; idx++)
    {
        ASSERT_TRUE(resultList[idx]);
    }

    ASSERT_TRUE(dir.exists());
}

TEST_F(AssetUtilitiesTest, CreateDir_InvalidDir_Timeout_Valid)
{
    QDir dir(":\folder");
    unsigned int timeToWaitInSecs = 1;
    AZStd::vector<AZStd::thread*> threadList;
    AZStd::vector<bool> resultList;
    AZStd::mutex resultMutex;
    auto runFunc = [&]()
    {
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));// sleeping to sync all the threads
        bool result = AssetUtilities::CreateDirectoryWithTimeout(dir, timeToWaitInSecs);
        AZStd::lock_guard<AZStd::mutex> locker(resultMutex);
        resultList.push_back(result);
    };

    int numberOfThreads = 5;

    ASSERT_FALSE(dir.exists());

    for (int idx = 0; idx < numberOfThreads; idx++)
    {
        AZStd::thread* workerThread = new AZStd::thread(runFunc);
        threadList.emplace_back(workerThread);
    }

    for (auto thread : threadList)
    {
        if (thread->joinable())
        {
            thread->join();
        }
        delete thread;
    }

    for (int idx = 0; idx < numberOfThreads; idx++)
    {
        ASSERT_FALSE(resultList[idx]);
    }

    ASSERT_FALSE(dir.exists());
}
