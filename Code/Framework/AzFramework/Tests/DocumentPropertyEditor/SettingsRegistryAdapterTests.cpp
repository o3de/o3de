/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/SystemFile.h>

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryOriginTracker.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzFramework/DocumentPropertyEditor/SettingsRegistryAdapter.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <Tests/DocumentPropertyEditor/DocumentPropertyEditorFixture.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/writer.h>


namespace AZ::DocumentPropertyEditor::Tests
{
    class SettingsRegistryAdapterDpeFixture : public DocumentPropertyEditorTestFixture
    {
    public:
        void SetUp() override
        {
            DocumentPropertyEditorTestFixture::SetUp();
            m_settingsRegistry = AZStd::make_unique<SettingsRegistryImpl>();
            m_settingsRegistryOriginTracker = AZStd::make_unique<SettingsRegistryOriginTracker>(*m_settingsRegistry);
            AZ::Interface<AZ::SettingsRegistryOriginTracker>::Register(m_settingsRegistryOriginTracker.get());
            m_adapter = AZStd::make_unique<SettingsRegistryAdapter>();
        }

        void TearDown() override
        {
            m_adapter.reset();
            AZ::Interface<AZ::SettingsRegistryOriginTracker>::Unregister(m_settingsRegistryOriginTracker.get());
            m_settingsRegistryOriginTracker.reset();
            m_settingsRegistry.reset();
            DocumentPropertyEditorTestFixture::TearDown();
        }

        static bool CreateTestFile(const AZ::IO::FixedMaxPath& testPath, AZStd::string_view content)
        {
            AZ::IO::SystemFile file;
            if (!file.Open(
                    testPath.c_str(),
                    AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH |
                        AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
            {
                AZ_Assert(false, "Unable to open test file for writing: %s", testPath.c_str());
                return false;
            }

            if (file.Write(content.data(), content.size()) != content.size())
            {
                AZ_Assert(false, "Unable to write content to test file: %s", testPath.c_str());
                return false;
            }

            return true;
        }

    protected:
        AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;
        AZStd::unique_ptr<AZ::SettingsRegistryOriginTracker> m_settingsRegistryOriginTracker;
        AZStd::unique_ptr<SettingsRegistryAdapter> m_adapter;
        AZ::Test::ScopedAutoTempDirectory m_testFolder;
    };

    TEST_F(SettingsRegistryAdapterDpeFixture, GenerateContent_MergedSettingsFiles_Displayed)
    {
        auto tempRootFolder = AZ::IO::FixedMaxPath(m_testFolder.GetDirectory());
        AZ::IO::FixedMaxPath filePath1 = tempRootFolder / "folder1" / "file1.json";
        AZ::IO::FixedMaxPath filePath2 = tempRootFolder / "folder2" / "file2.json";
        ASSERT_TRUE(CreateTestFile(filePath1, R"(
           {
               "O3DE": {
                    "ArrayValue": [
                        3,
                        7,
                        4000
                    ],
                    "ObjectValue": {
                        "StringKey1": "Hello",
                        "BoolKey1": true
                    }
               }
           }
        )"));
        ASSERT_TRUE(CreateTestFile(filePath2, R"(
           {
               "O3DE": {
                    "ArrayValue": [
                        27,
                        39
                    ],
                    "ObjectValue": {
                        "StringKey1": "Hi",
                        "IntKey2": 9001,
                    },
                    "DoubleValue": 4.0
                }
           }
        )"));
        m_settingsRegistry->MergeSettingsFile(
            filePath1.FixedMaxPathString(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "", nullptr);
        m_settingsRegistry->MergeSettingsFile(
            filePath2.FixedMaxPathString(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "", nullptr);

        Dom::Value result = m_adapter->GetContents();
        AZStd::string_view stringKey1Path = "/O3DE/ObjectValue/StringKey1";
        AZStd::string_view stringKey1NewValue = R"(Greetings)";
        ASSERT_TRUE(m_settingsRegistry->Set(stringKey1Path, stringKey1NewValue));
        result = m_adapter->GetContents();
    }
}
