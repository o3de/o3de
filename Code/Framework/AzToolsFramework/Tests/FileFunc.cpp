/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <FrameworkApplicationFixture.h>
#include <Utils/Utils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTest/AzTest.h>

#include <QTemporaryDir>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>

namespace UnitTest
{
    class FileFuncTest : public ScopedAllocatorSetupFixture
    {
    public:
        void SetUp() override
        {
            m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(&m_fileIO);
        }

        void TearDown() override
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
        }

        AZ::IO::LocalFileIO m_fileIO;
        AZ::IO::FileIOBase* m_prevFileIO;
    };

    static bool CreateDummyFile(const QString& fullPathToFile, const QString& tempStr = {})
    {
        QFileInfo fi(fullPathToFile);
        QDir fp(fi.path());
        fp.mkpath(".");
        QFile writer(fullPathToFile);
        if (!writer.open(QFile::ReadWrite))
        {
            return false;
        }
        {
            QTextStream stream(&writer);
            stream << tempStr << Qt::endl;
        }
        writer.close();
        return true;
    }

    TEST_F(FileFuncTest, FindFilesTest_EmptyFolder_Failure)
    {
        QTemporaryDir tempDir;

        QDir tempPath(tempDir.path());

        const char dependenciesPattern[] = "*_dependencies.xml";
        bool recurse = true;
        AZStd::string folderPath = tempPath.absolutePath().toStdString().c_str();
        AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> result = AzFramework::FileFunc::FindFileList(folderPath.c_str(),
            dependenciesPattern, recurse);

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_EQ(result.GetValue().size(), 0);
    }

    TEST_F(FileFuncTest, FindFilesTest_DependenciesWildcards_Success)
    {
        QTemporaryDir tempDir;

        QDir tempPath(tempDir.path());

        const char* expectedFileNames[] = { "a_dependencies.xml","b_dependencies.xml" };
        ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath(expectedFileNames[0]), QString("tempdata\n")));
        ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath(expectedFileNames[1]), QString("tempdata\n")));
        ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath("dependencies.xml"), QString("tempdata\n")));

        const char dependenciesPattern[] = "*_dependencies.xml";
        bool recurse = true;
        AZStd::string folderPath = tempPath.absolutePath().toStdString().c_str();
        AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> result = AzFramework::FileFunc::FindFileList(folderPath.c_str(),
            dependenciesPattern, recurse);

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_EQ(result.GetValue().size(), 2);

        for (size_t i = 0; i < AZ_ARRAY_SIZE(expectedFileNames); ++i)
        {
            auto findElement = AZStd::find_if(result.GetValue().begin(), result.GetValue().end(), [&expectedFileNames, i](const AZStd::string& thisString)
            {
                AZStd::string thisFileName;
                AzFramework::StringFunc::Path::GetFullFileName(thisString.c_str(), thisFileName);
                return thisFileName == expectedFileNames[i];
            });
            ASSERT_NE(findElement, result.GetValue().end());
        }
    }

    TEST_F(FileFuncTest, FindFilesTest_DependenciesWildcardsSubfolders_Success)
    {
        QTemporaryDir tempDir;

        QDir tempPath(tempDir.path());

        ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath("a_dependencies.xml"), QString("tempdata\n")));
        ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath("b_dependencies.xml"), QString("tempdata\n")));
        ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath("dependencies.xml"), QString("tempdata\n")));

        const char dependenciesPattern[] = "*_dependencies.xml";
        bool recurse = true;
        AZStd::string folderPath = tempPath.absolutePath().toStdString().c_str();

        ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath("subfolder1/c_dependencies.xml"), QString("tempdata\n")));
        ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath("subfolder1/d_dependencies.xml"), QString("tempdata\n")));
        ASSERT_TRUE(CreateDummyFile(tempPath.absoluteFilePath("subfolder1/dependencies.xml"), QString("tempdata\n")));

        AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> result = AzFramework::FileFunc::FindFileList(folderPath.c_str(),
            dependenciesPattern, recurse);

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_EQ(result.GetValue().size(), 4);

        const char* expectedFileNames[] = { "a_dependencies.xml","b_dependencies.xml", "c_dependencies.xml", "d_dependencies.xml" };
        for (size_t i = 0; i < AZ_ARRAY_SIZE(expectedFileNames); ++i)
        {
            auto findElement = AZStd::find_if(result.GetValue().begin(), result.GetValue().end(), [&expectedFileNames, i](const AZStd::string& thisString)
            {
                AZStd::string thisFileName;
                AzFramework::StringFunc::Path::GetFullFileName(thisString.c_str(), thisFileName);
                return thisFileName == expectedFileNames[i];
            });
            ASSERT_NE(findElement, result.GetValue().end());
        }
    }
} // namespace UnitTest
