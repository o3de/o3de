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

    protected:
        AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;
        AZStd::unique_ptr<AZ::SettingsRegistryOriginTracker> m_settingsRegistryOriginTracker;
        AZStd::unique_ptr<SettingsRegistryAdapter> m_adapter;
        AZ::Test::ScopedAutoTempDirectory m_testFolder;
    };

    TEST_F(SettingsRegistryAdapterDpeFixture, GenerateContent_MergedSettingsFiles_Displayed)
    {
        constexpr AZ::IO::PathView filePath1 = "folder1/file1.json";
        constexpr AZ::IO::PathView filePath2 = "folder2/file2.json";
        ASSERT_TRUE(AZ::Test::CreateTestFile(m_testFolder, filePath1, R"(
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
        ASSERT_TRUE(AZ::Test::CreateTestFile(m_testFolder, filePath2, R"(
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

        auto tempRootFolder = m_testFolder.GetDirectoryAsFixedMaxPath();
        m_settingsRegistry->MergeSettingsFile(
            (tempRootFolder / filePath1).Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "", nullptr);
        m_settingsRegistry->MergeSettingsFile(
            (tempRootFolder / filePath2).Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "", nullptr);

        Dom::Value result = m_adapter->GetContents();
        AZStd::string_view stringKey1Path = "/O3DE/ObjectValue/StringKey1";
        AZStd::string_view stringKey1NewValue = R"(Greetings)";
        ASSERT_TRUE(m_settingsRegistry->Set(stringKey1Path, stringKey1NewValue));
        result = m_adapter->GetContents();
    }
}
