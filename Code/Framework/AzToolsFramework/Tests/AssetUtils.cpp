/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/utils.h>
#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Asset/AssetUtils.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace //anonymous
{
    const char DummyProjectName[] = "DummyProject";
    const char GemsFolder[] = "Gems";
    const char GemAName[] = "GemA";
    const char GemBName[] = "GemB";
    const char GemCName[] = "GemC";

    constexpr int TotalNumberFiles = 5;
    const char* FileNames[TotalNumberFiles] = { "gems.json" , "project.json" , "gem.json", "gem.json" , "gem.json" };
    const int FileHandles[TotalNumberFiles] = { 1111, 2222, 3333, 4444, 5555 };

    const int GemsIdx = 0;
    const int ProjectIdx = 1;
    const int GemAGemIdx = 2;
    const int GemBGemIdx = 3;
    const int GemCGemIdx = 4;


    const char GemsFileContent[] = R"({
        "GemListFormatVersion": 2,
        "Gems" : [
    {
        "Path": "Gems/GemA",
            "Uuid" : "044a63ea67d04479aa5daf62ded9d9cb",
            "Version" : "0.1.0",
            "_comment" : "GemA"
    },
        {
            "Path": "Gems/GemB",
            "Uuid" : "07375b61b1a2424bb03088bbdf28b2c9",
            "Version" : "0.1.0",
            "_comment" : "GemB"
        },
        {
            "Path": "Gems/GemC",
            "Uuid" : "0945e21b7ae848ac80b4ec1f34c459cd",
            "Version" : "0.1.0",
            "_comment" : "GemC"
        }
        ]
})";

    const char ProjectFileContent[] = R"({
    "project_name": "DummyProject",
    "product_name": "DummyProject",
    "executable_name": "DummyProjectLauncher",
    "modules" : [],
    "project_id": "{91FB81A1-072C-4A80-8FCC-7E2C4C767B4D}",

    "android_settings" : {
        "package_name" : "com.lumberyard.yourgame",
        "version_number" : 1,
        "version_name" : "1.0.0.0",
        "orientation" : "landscape"
    },

    "provo_settings": {
    }

})";

    const char GemAGemFileContent[] = R"({
    "GemFormatVersion": 4,
    "Uuid": "044A63EA67D04479AA5DAF62DED9D9CB",
    "Name": "GemA",
    "DisplayName": "GemA",
    "Version": "0.1.0",
    "Summary": "Only for unit test purposes.",
    "Tags": ["Foo"],
    "IconPath": "preview.png",
    "EditorModule": true
})";

    const char GemBGemFileContent[] = R"({
    "GemFormatVersion": 4,
    "Uuid": "07375B61B1A2424BB03088BBDF28B2C9",
    "Name": "GemB",
    "DisplayName": "GemB",
    "Version": "0.1.0",
    "Summary": "Only for unit test purposes.",
    "Tags": ["Foo"],
    "IconPath": "preview.png",
    "EditorModule": true
})";

    const char GemCGemFileContent[] = R"({
        "GemFormatVersion": 4,
            "Uuid" : "0945E21B7AE848AC80B4EC1F34C459CD",
            "Name" : "GemC",
            "DisplayName" : "GemC",
            "Version" : "0.1.0",
            "Summary" : "Only for unit test purposes.",
            "Tags" : ["Foo"],
            "IconPath" : "preview.png",
            "EditorModule" : true
    })";

    const char* FileContents[TotalNumberFiles] = { GemsFileContent , ProjectFileContent, GemAGemFileContent, GemBGemFileContent, GemCGemFileContent };

}

namespace UnitTest
{
    using ::testing::NiceMock;
    using ::testing::_;
    using ::testing::Return;

    class MockFileIO
        : public AZ::IO::MockFileIOBase
    {
    public:
        MockFileIO()
        {
            PopulateData();
            SetupMocks();
        }

        void PopulateData()
        {
            AZStd::string gemsSettingsFilePath;
            AzFramework::StringFunc::Path::Join(DummyProjectName, FileNames[GemsIdx], gemsSettingsFilePath);

            m_fileHandleContentMap[AZ::Uuid::CreateName(gemsSettingsFilePath.c_str())] = AZStd::make_pair(FileHandles[GemsIdx], FileContents[GemsIdx]);

            AZStd::string projectSettingsFilePath;
            AzFramework::StringFunc::Path::Join(DummyProjectName, FileNames[ProjectIdx], projectSettingsFilePath);

            m_fileHandleContentMap[AZ::Uuid::CreateName(projectSettingsFilePath.c_str())] = AZStd::make_pair(FileHandles[ProjectIdx], FileContents[ProjectIdx]);

            AZStd::string gemAGemFilePath;
            AzFramework::StringFunc::Path::Join(GemsFolder, GemAName, gemAGemFilePath);
            AzFramework::StringFunc::Path::Join(gemAGemFilePath.c_str(), FileNames[GemAGemIdx], gemAGemFilePath);

            m_fileHandleContentMap[AZ::Uuid::CreateName(gemAGemFilePath.c_str())] = AZStd::make_pair(FileHandles[GemAGemIdx], FileContents[GemAGemIdx]);

            AZStd::string gemBGemFilePath;
            AzFramework::StringFunc::Path::Join(GemsFolder, GemBName, gemBGemFilePath);
            AzFramework::StringFunc::Path::Join(gemBGemFilePath.c_str(), FileNames[GemBGemIdx], gemBGemFilePath);

            m_fileHandleContentMap[AZ::Uuid::CreateName(gemBGemFilePath.c_str())] = AZStd::make_pair(FileHandles[GemBGemIdx], FileContents[GemBGemIdx]);

            AZStd::string gemCGemFilePath;
            AzFramework::StringFunc::Path::Join(GemsFolder, GemCName, gemCGemFilePath);
            AzFramework::StringFunc::Path::Join(gemCGemFilePath.c_str(), FileNames[GemCGemIdx], gemCGemFilePath);

            m_fileHandleContentMap[AZ::Uuid::CreateName(gemCGemFilePath.c_str())] = AZStd::make_pair(FileHandles[GemCGemIdx], FileContents[GemCGemIdx]);
        }

        void SetupMocks()
        {
            ON_CALL(*this, Open(_, _, _)).WillByDefault(testing::Invoke(
                [&](const char* filePath, AZ::IO::OpenMode mode, AZ::IO::HandleType& fileHandle)
                {
                    auto found = m_fileHandleContentMap.find(AZ::Uuid::CreateName(filePath));
                    if (found == m_fileHandleContentMap.end())
                    {
                        return AZ::IO::Result(AZ::IO::ResultCode::Error);
                    }

                    fileHandle = found->second.first;
                    return AZ::IO::Result(AZ::IO::ResultCode::Success);
                }
            ));

            ON_CALL(*this, Read(_, _, _, _, _)).WillByDefault(testing::Invoke(
                [&](AZ::IO::HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead, AZ::u64* bytesRead)
                {
                    for (auto iter = m_fileHandleContentMap.begin(); iter != m_fileHandleContentMap.end(); iter++)
                    {
                        if (iter->second.first == fileHandle)
                        {
                            memcpy(buffer, iter->second.second.c_str(), iter->second.second.length() + 1);
                            return AZ::IO::Result(AZ::IO::ResultCode::Success);
                        }
                    }

                    return AZ::IO::Result(AZ::IO::ResultCode::Error);
                }
            ));

            ON_CALL(*this, Size(testing::Matcher<AZ::IO::HandleType>(_), testing::Matcher<AZ::u64&>(_))).WillByDefault(testing::Invoke(
                [&](AZ::IO::HandleType fileHandle, AZ::u64& size)
                {
                    for (auto iter = m_fileHandleContentMap.begin(); iter != m_fileHandleContentMap.end(); iter++)
                    {
                        if (iter->second.first == fileHandle)
                        {
                            size = iter->second.second.length();
                            return AZ::IO::Result(AZ::IO::ResultCode::Success);
                        }
                    }

                    return AZ::IO::Result(AZ::IO::ResultCode::Error);
                }
            ));

            ON_CALL(*this, Exists(_)).WillByDefault(testing::Invoke(
                [&](const char* filePath)
                {
                    auto found = m_fileHandleContentMap.find(AZ::Uuid::CreateName(filePath));
                    if (found == m_fileHandleContentMap.end())
                    {
                        return false;
                    }

                    return true;
                }
            ));

            ON_CALL(*this, Close(_))
                .WillByDefault(
                    Return(AZ::IO::Result(AZ::IO::ResultCode::Success)));

        }

        AZStd::unordered_map<AZ::Uuid, AZStd::pair<AZ::IO::HandleType, AZStd::string>> m_fileHandleContentMap;

    };

    class AssetUtilitiesGemsTest
        : public ::testing::Test
    {
    public:
        void SetUp() override
        {
            m_data = AZStd::make_unique<StaticData>();

            m_data->m_priorFileIO = AZ::IO::FileIOBase::GetInstance();

            m_data->m_application.reset(aznew AzToolsFramework::ToolsApplication);
            m_data->m_application->Start(AzFramework::Application::Descriptor());
            m_data->m_localFileIO = new ::testing::NiceMock<MockFileIO>();
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_data->m_localFileIO);
        }
        void TearDown() override
        {
            delete m_data->m_localFileIO;
            AZ::IO::FileIOBase::SetInstance(m_data->m_priorFileIO);
            m_data->m_application->Stop();
            m_data->m_application.reset();
            m_data.reset();
        }

        struct StaticData
        {
            AZStd::unique_ptr<MockFileIO> m_fileIO;
            AZStd::string m_testEngineRoot;
            AZStd::unique_ptr<AzToolsFramework::ToolsApplication> m_application = {};
            AZStd::vector<AzToolsFramework::AssetUtils::GemInfo> m_gemInfoList;
            AZ::IO::FileIOBase* m_priorFileIO = nullptr;
            AZ::IO::FileIOBase* m_localFileIO = nullptr;
        };

        AZStd::unique_ptr<StaticData> m_data;
    };

    TEST_F(AssetUtilitiesGemsTest, GemSystem_RetreiveGemsList_OK)
    {
        AzToolsFramework::AssetUtils::GetGemsInfo(m_data->m_testEngineRoot.c_str(), m_data->m_testEngineRoot.c_str(), DummyProjectName, m_data->m_gemInfoList);
        EXPECT_EQ(m_data->m_gemInfoList.size(), 3);
        AZStd::unordered_set<AZStd::string> gemsNameMap{ "GemA", "GemB", "GemC" };
        for (const AzToolsFramework::AssetUtils::GemInfo& gemInfo : m_data->m_gemInfoList)
        {
            gemsNameMap.erase(gemInfo.m_gemName);
        }

        ASSERT_EQ(gemsNameMap.size(), 0);
    }
}
