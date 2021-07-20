/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FrameworkApplicationFixture.h"
#include "Utils/Utils.h"
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

namespace AzFramework
{
    namespace FileFunc
    {
        namespace Internal
        {
            AZ::Outcome<void,AZStd::string> UpdateCfgContents(AZStd::string& cfgContents, const AZStd::list<AZStd::string>& updateRules);
            AZ::Outcome<void,AZStd::string> UpdateCfgContents(AZStd::string& cfgContents, const AZStd::string& header, const AZStd::string& key, const AZStd::string& value);
            AZ::Outcome<void, AZStd::string> WriteJsonToStream(const rapidjson::Document& document, AZ::IO::GenericStream& stream,
                WriteJsonSettings settings = WriteJsonSettings{});
        }
    }
}

namespace UnitTest
{
    class FileFuncTest : public ScopedAllocatorSetupFixture
    {
    public:
        void SetUp()
        {
            m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(&m_fileIO);
        }

        void TearDown() override
        {
            AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
        }

        AZ::IO::LocalFileIO m_fileIO;
        AZ::IO::FileIOBase* m_prevFileIO;
    };

    TEST_F(FileFuncTest, UpdateCfgContents_InValidInput_Fail)
    {
        AZStd::string               cfgContents = "[Foo]\n";
        AZStd::list<AZStd::string>  updateRules;

        updateRules.push_back(AZStd::string("Foo/one*1"));
        auto result = AzFramework::FileFunc::Internal::UpdateCfgContents(cfgContents, updateRules);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(FileFuncTest, UpdateCfgContents_ValidInput_Success)
    {
        AZStd::string cfgContents =
            "[Foo]\n"
            "one =2 \n"
            "two= 3\n"
            "three = 4\n"
            "\n"
            "[Bar]\n"
            "four=3\n"
            "five=3\n"
            "six=3\n"
            "eight=3\n";

        AZStd::list<AZStd::string> updateRules;

        updateRules.push_back(AZStd::string("Foo/one=1"));
        updateRules.push_back(AZStd::string("Foo/two=2"));
        updateRules.push_back(AZStd::string("three=3"));
        auto result = AzFramework::FileFunc::Internal::UpdateCfgContents(cfgContents, updateRules);
        EXPECT_TRUE(result.IsSuccess());

        AZStd::string compareCfgContents =
            "[Foo]\n"
            "one =1\n"
            "two= 2\n"
            "three = 3\n"
            "\n"
            "[Bar]\n"
            "four=3\n"
            "five=3\n"
            "six=3\n"
            "eight=3\n";

        bool equals = cfgContents.compare(compareCfgContents) == 0;
        ASSERT_TRUE(equals);
    }

    TEST_F(FileFuncTest, UpdateCfgContents_ValidInputNewEntrySameHeader_Success)
    {
        AZStd::string cfgContents =
            "[Foo]\n"
            "one =2 \n"
            "two= 3\n"
            "three = 4\n";

        AZStd::string header("[Foo]");
        AZStd::string key("four");
        AZStd::string value("4");
        auto result = AzFramework::FileFunc::Internal::UpdateCfgContents(cfgContents, header, key, value);
        EXPECT_TRUE(result.IsSuccess());

        AZStd::string compareCfgContents =
            "[Foo]\n"
            "four=4\n"
            "one =2 \n"
            "two= 3\n"
            "three = 4\n";

        bool equals = cfgContents.compare(compareCfgContents) == 0;
        ASSERT_TRUE(equals);
    }

    TEST_F(FileFuncTest, UpdateCfgContents_ValidInputNewEntryDifferentHeader_Success)
    {
        AZStd::string cfgContents =
            ";Sample Data\n"
            "[Foo]\n"
            "one =2 \n"
            "two= 3\n"
            "three = 4\n";

        AZStd::list<AZStd::string> updateRules;

        AZStd::string header("[Bar]");
        AZStd::string key("four");
        AZStd::string value("4");
        auto result = AzFramework::FileFunc::Internal::UpdateCfgContents(cfgContents, header, key, value);
        EXPECT_TRUE(result.IsSuccess());

        AZStd::string compareCfgContents =
            ";Sample Data\n"
            "[Foo]\n"
            "one =2 \n"
            "two= 3\n"
            "three = 4\n"
            "\n"
            "[Bar]\n"
            "four=4\n";

        bool equals = cfgContents.compare(compareCfgContents) == 0;
        ASSERT_TRUE(equals);
    }

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

    class JsonFileFuncTest
        : public FrameworkApplicationFixture
    {
    protected:
        void SetUp() override
        {
            FrameworkApplicationFixture::SetUp();

            AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
            auto projectPathKey =
                AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
            registry->Set(projectPathKey, "AutomatedTesting");
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
            m_jsonSystemComponent = AZStd::make_unique<AZ::JsonSystemComponent>();

            m_serializationSettings.m_serializeContext = m_serializeContext.get();
            m_serializationSettings.m_registrationContext = m_jsonRegistrationContext.get();

            m_deserializationSettings.m_serializeContext = m_serializeContext.get();
            m_deserializationSettings.m_registrationContext = m_jsonRegistrationContext.get();

            m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
        }

        void TearDown() override
        {
            m_jsonRegistrationContext->EnableRemoveReflection();
            m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
            m_jsonRegistrationContext->DisableRemoveReflection();

            m_jsonRegistrationContext.reset();
            m_serializeContext.reset();
            m_jsonSystemComponent.reset();

            FrameworkApplicationFixture::TearDown();
        }

        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;
        AZStd::unique_ptr<AZ::JsonSystemComponent> m_jsonSystemComponent;

        AZ::JsonSerializerSettings m_serializationSettings;
        AZ::JsonDeserializerSettings m_deserializationSettings;
    };

    TEST_F(JsonFileFuncTest, WriteJsonString_ValidJson_ExpectSuccess)
    {
        rapidjson::Document document;
        document.SetObject();
        document.AddMember("a", 1, document.GetAllocator());
        document.AddMember("b", 2, document.GetAllocator());
        document.AddMember("c", 3, document.GetAllocator());

        AZStd::string expectedJsonText =
            R"({
                "a": 1,
                "b": 2,
                "c": 3
            })";
        expectedJsonText.erase(AZStd::remove_if(expectedJsonText.begin(), expectedJsonText.end(), ::isspace), expectedJsonText.end());

        AZStd::string outString;
        AZ::Outcome<void, AZStd::string> result = AzFramework::FileFunc::WriteJsonToString(document, outString);
        EXPECT_TRUE(result.IsSuccess());

        outString.erase(AZStd::remove_if(outString.begin(), outString.end(), ::isspace), outString.end());
        EXPECT_EQ(expectedJsonText, outString) << "expected:\n" << expectedJsonText.c_str() << "\nactual:\n" << outString.c_str();
    }

    TEST_F(JsonFileFuncTest, WriteJsonStream_ValidJson_ExpectSuccess)
    {
        rapidjson::Document document;
        document.SetObject();
        document.AddMember("a", 1, document.GetAllocator());
        document.AddMember("b", 2, document.GetAllocator());
        document.AddMember("c", 3, document.GetAllocator());

        AZStd::string expectedJsonText =
            R"({
                "a": 1,
                "b": 2,
                "c": 3
            })";
        expectedJsonText.erase(AZStd::remove_if(expectedJsonText.begin(), expectedJsonText.end(), ::isspace), expectedJsonText.end());

        AZStd::vector<char> outBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char>> outStream{ &outBuffer };
        AZ::Outcome<void, AZStd::string> result = AzFramework::FileFunc::Internal::WriteJsonToStream(document, outStream);
        EXPECT_TRUE(result.IsSuccess());

        outBuffer.push_back(0);
        AZStd::string outString = outBuffer.data();
        outString.erase(AZStd::remove_if(outString.begin(), outString.end(), ::isspace), outString.end());
        EXPECT_EQ(expectedJsonText, outString) << "expected:\n" << expectedJsonText.c_str() << "\nactual:\n" << outString.c_str();
    }

    TEST_F(JsonFileFuncTest, WriteJsonFile_ValidJson_ExpectSuccess)
    {
        AZ::Test::ScopedAutoTempDirectory tempDir;

        rapidjson::Document document;
        document.SetObject();
        document.AddMember("a", 1, document.GetAllocator());
        document.AddMember("b", 2, document.GetAllocator());
        document.AddMember("c", 3, document.GetAllocator());

        AZStd::string expectedJsonText =
            R"({
                "a": 1,
                "b": 2,
                "c": 3
            })";
        expectedJsonText.erase(AZStd::remove_if(expectedJsonText.begin(), expectedJsonText.end(), ::isspace), expectedJsonText.end());

        AZStd::string pathStr;
        AzFramework::StringFunc::Path::ConstructFull(tempDir.GetDirectory(), "test.json", pathStr, true);

        // Write the JSON to a file
        AZ::IO::Path path(pathStr);
        AZ::Outcome<void, AZStd::string> saveResult = AzFramework::FileFunc::WriteJsonFile(document, path);
        EXPECT_TRUE(saveResult.IsSuccess());

        // Verify that the contents of the file is what we expect
        AZ::Outcome<AZStd::string, AZStd::string> readResult = AZ::Utils::ReadFile(pathStr);
        EXPECT_TRUE(readResult.IsSuccess());
        AZStd::string outString(readResult.TakeValue());
        outString.erase(AZStd::remove_if(outString.begin(), outString.end(), ::isspace), outString.end());
        EXPECT_EQ(outString, expectedJsonText);

        // Clean up 
        AZ::IO::FileIOBase::GetInstance()->Remove(path.c_str());
    }

    TEST_F(JsonFileFuncTest, ReadJsonString_ValidJson_ExpectSuccess)
    {
        const char* jsonText =
            R"(
            {
                "a": 1,
                "b": 2,
                "c": 3
            })";

        AZ::Outcome<rapidjson::Document, AZStd::string> result = AzFramework::FileFunc::ReadJsonFromString(jsonText);

        EXPECT_TRUE(result.IsSuccess());
        EXPECT_TRUE(result.GetValue().IsObject());
        EXPECT_TRUE(result.GetValue().HasMember("a"));
        EXPECT_TRUE(result.GetValue().HasMember("b"));
        EXPECT_TRUE(result.GetValue().HasMember("c"));
        EXPECT_EQ(result.GetValue()["a"].GetInt(), 1);
        EXPECT_EQ(result.GetValue()["b"].GetInt(), 2);
        EXPECT_EQ(result.GetValue()["c"].GetInt(), 3);
    }

    TEST_F(JsonFileFuncTest, ReadJsonString_InvalidJson_ErrorReportsLineNumber)
    {
        const char* jsonText =
            R"(
            {
                "a": "This line is missing a comma"
                "b": 2,
                "c": 3
            }
            )";

        AZ::Outcome<rapidjson::Document, AZStd::string> result = AzFramework::FileFunc::ReadJsonFromString(jsonText);

        EXPECT_FALSE(result.IsSuccess());
        EXPECT_TRUE(result.GetError().find("JSON parse error at line 4:") == 0);
    }

    TEST_F(JsonFileFuncTest, ReadJsonFile_ValidJson_ExpectSuccess)
    {
        AZ::Test::ScopedAutoTempDirectory tempDir;

        const char* inputJsonText =
            R"({
                "a": 1,
                "b": 2,
                "c": 3
            })";

        rapidjson::Document expectedDocument;
        expectedDocument.SetObject();
        expectedDocument.AddMember("a", 1, expectedDocument.GetAllocator());
        expectedDocument.AddMember("b", 2, expectedDocument.GetAllocator());
        expectedDocument.AddMember("c", 3, expectedDocument.GetAllocator());

        // Create test file
        AZStd::string path;
        AzFramework::StringFunc::Path::ConstructFull(tempDir.GetDirectory(), "test.json", path, true);
        AZ::Outcome<void, AZStd::string> writeResult = AZ::Utils::WriteFile(inputJsonText, path);
        EXPECT_TRUE(writeResult.IsSuccess());


        // Read the JSON from the test file
        AZ::Outcome<rapidjson::Document, AZStd::string> readResult = AzFramework::FileFunc::ReadJsonFile(path);
        EXPECT_TRUE(readResult.IsSuccess());

        EXPECT_EQ(expectedDocument, readResult.GetValue());

        // Clean up 
        AZ::IO::FileIOBase::GetInstance()->Remove(path.c_str());
    }
} // namespace UnitTest
