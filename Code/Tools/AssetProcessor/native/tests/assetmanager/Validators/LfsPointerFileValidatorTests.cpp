/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/
#include <native/tests/assetmanager/Validators/LfsPointerFileValidatorTests.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

namespace UnitTests
{
    void LfsPointerFileValidatorTests::SetUp()
    {
        AssetManagerTestingBase::SetUp();

        m_assetRootDir = m_databaseLocationListener.GetAssetRootDir();
        CreateTestFile((m_assetRootDir / ".gitattributes").Native(),
            "#\n"
            "# Git LFS(see https ://git-lfs.github.com/)\n"
            "#\n"
            "*.test filter=lfs diff=lfs merge=lfs -text\n"
        );

        m_validator = AssetProcessor::LfsPointerFileValidator({ m_assetRootDir.c_str() });
    }

    void LfsPointerFileValidatorTests::TearDown()
    {
        RemoveTestFile((m_assetRootDir / ".gitattributes").Native());
        AssetManagerTestingBase::TearDown();
    }

    bool LfsPointerFileValidatorTests::CreateTestFile(const AZStd::string& filePath, const AZStd::string& content)
    {
        AZ::IO::HandleType fileHandle;
        if (!AZ::IO::FileIOBase::GetInstance()->Open(filePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText, fileHandle))
        {
            return false;
        }

        AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, content.c_str(), content.size());
        AZ::IO::FileIOBase::GetInstance()->Close(fileHandle);
        return true;
    }

    bool LfsPointerFileValidatorTests::RemoveTestFile(const AZStd::string& filePath)
    {
        if (AZ::IO::FileIOBase::GetInstance()->Exists(filePath.c_str()))
        {
            return AZ::IO::FileIOBase::GetInstance()->Remove(filePath.c_str());
        }

        return true;
    }

    TEST_F(LfsPointerFileValidatorTests, GetLfsPointerFilePathPatterns_GitAttributesFileExists_ReturnLfsPointerFilePathPatterns)
    {
        AZStd::set<AZStd::string> lfsPointerFilePathPatterns = m_validator.GetLfsPointerFilePathPatterns();
        ASSERT_EQ(lfsPointerFilePathPatterns.size(), 1);
        ASSERT_EQ(*lfsPointerFilePathPatterns.begin(), "*.test");
    }

    TEST_F(LfsPointerFileValidatorTests, IsLfsPointerFile_ValidLfsPointerFile_CheckSucceed)
    {
        AZStd::string testFilePath = (m_assetRootDir / "file.test").Native();
        CreateTestFile(testFilePath,
            "version https://git-lfs.github.com/spec/v1\n"
            "oid sha256:ee4799379bfcfa99e95afd6494da51fbeda95f21ea71d267ae7102f048edec85\n"
            "size 63872\n");

        ASSERT_TRUE(m_validator.IsLfsPointerFile(testFilePath));

        RemoveTestFile(testFilePath);
    }

    TEST_F(LfsPointerFileValidatorTests, IsLfsPointerFile_NonLfsPointerFileType_CheckFail)
    {
        AZStd::string testFilePath = (m_assetRootDir / "file.test1").Native();
        CreateTestFile(testFilePath,
            "version https://git-lfs.github.com/spec/v1\n"
            "oid sha256:ee4799379bfcfa99e95afd6494da51fbeda95f21ea71d267ae7102f048edec85\n"
            "size 63872\n");

        ASSERT_FALSE(m_validator.IsLfsPointerFile(testFilePath));

        RemoveTestFile(testFilePath);
    }

    TEST_F(LfsPointerFileValidatorTests, IsLfsPointerFile_InvalidFirstKey_CheckFail)
    {
        AZStd::string testFilePath = (m_assetRootDir / "file.test").Native();
        CreateTestFile(testFilePath,
            "oid sha256:ee4799379bfcfa99e95afd6494da51fbeda95f21ea71d267ae7102f048edec85\n"
            "size 63872\n"
            "version https://git-lfs.github.com/spec/v1\n");

        ASSERT_FALSE(m_validator.IsLfsPointerFile(testFilePath));

        RemoveTestFile(testFilePath);
    }

    TEST_F(LfsPointerFileValidatorTests, IsLfsPointerFile_InvalidKeyCharacter_CheckFail)
    {
        AZStd::string testFilePath = (m_assetRootDir / "file.test").Native();
        CreateTestFile(testFilePath,
            "version https://git-lfs.github.com/spec/v1\n"
            "oid+ sha256:ee4799379bfcfa99e95afd6494da51fbeda95f21ea71d267ae7102f048edec85\n"
            "size 63872\n");

        ASSERT_FALSE(m_validator.IsLfsPointerFile(testFilePath));

        RemoveTestFile(testFilePath);
    }

    TEST_F(LfsPointerFileValidatorTests, IsLfsPointerFile_UnorderedKeys_CheckFail)
    {
        AZStd::string testFilePath = (m_assetRootDir / "file.test").Native();
        CreateTestFile(testFilePath,
            "version https://git-lfs.github.com/spec/v1\n"
            "size 63872\n"
            "oid sha256:ee4799379bfcfa99e95afd6494da51fbeda95f21ea71d267ae7102f048edec85\n");

        ASSERT_FALSE(m_validator.IsLfsPointerFile(testFilePath));

        RemoveTestFile(testFilePath);
    }

    TEST_F(LfsPointerFileValidatorTests, IsLfsPointerFile_MissingRequiredKey_CheckFail)
    {
        AZStd::string testFilePath = (m_assetRootDir / "file.test").Native();
        CreateTestFile(testFilePath,
            "version https://git-lfs.github.com/spec/v1\n"
            "bla 63872\n"
            "oid sha256:ee4799379bfcfa99e95afd6494da51fbeda95f21ea71d267ae7102f048edec85\n");

        ASSERT_FALSE(m_validator.IsLfsPointerFile(testFilePath));

        RemoveTestFile(testFilePath);
    }
}
