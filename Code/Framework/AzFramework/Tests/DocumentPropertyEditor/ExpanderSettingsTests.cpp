/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/DocumentPropertyEditor/DocumentPropertyEditorFixture.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/DocumentPropertyEditor/ExpanderSettings.h>
#include <AzTest/Utils.h>

namespace AZ::DocumentPropertyEditor::Tests
{
    // ----------------------------------------------------------------------------------
    // Fixture: a settings registry and a project folder of the test's own
    // ----------------------------------------------------------------------------------

    class ExpanderSettingsDpeTests : public DocumentPropertyEditorTestFixture
    {
    public:
        void SetUp() override
        {
            DocumentPropertyEditorTestFixture::SetUp();
            CreateRegistry();
        }

        void TearDown() override
        {
            DestroyRegistry();
            DocumentPropertyEditorTestFixture::TearDown();
        }

        void CreateRegistry()
        {
            m_registry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            AZ::SettingsRegistry::Register(m_registry.get());
            m_registry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath, m_projectFolder.GetDirectory());
        }

        void DestroyRegistry()
        {
            AZ::SettingsRegistry::Unregister(m_registry.get());
            m_registry.reset();
        }

        //! The settings file of the property editor "Test", holding one marker value
        void WriteSettingsFile(int marker)
        {
            const AZStd::string contents =
                AZStd::string::format(R"({ "O3DE": { "DocumentPropertyEditor": { "Test": { "Marker": %d } } } })", marker);
            ASSERT_TRUE(AZ::Test::CreateTestFile(m_projectFolder, "user/Registry/DocumentPropertyEditor/Test_settings.setreg", contents));
        }

        AZ::s64 Marker() const
        {
            AZ::s64 marker = -1;
            m_registry->Get(marker, MarkerKey);
            return marker;
        }

        //! The expander settings of one component editor of the property editor "Test": constructing them loads the settings file
        void CreateSettings()
        {
            ExpanderSettings settings(m_adapter.get(), "uuid1", "Test");
        }

        static constexpr const char* MarkerKey = "/O3DE/DocumentPropertyEditor/Test/Marker";
        AZ::Test::ScopedAutoTempDirectory m_projectFolder;
        AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_registry;
    };

    // ----------------------------------------------------------------------------------
    // Loading the settings file
    // ----------------------------------------------------------------------------------

    TEST_F(ExpanderSettingsDpeTests, Load_TheSettingsFileIsMergedOnceWhileItIsUnchanged)
    {
        WriteSettingsFile(1);
        CreateSettings();
        EXPECT_EQ(Marker(), 1);

        // a value that only the registry holds: merging the unchanged file again would replace it
        m_registry->Set(MarkerKey, AZ::s64{ 2 });
        CreateSettings();
        CreateSettings();
        EXPECT_EQ(Marker(), 2);
    }

    TEST_F(ExpanderSettingsDpeTests, Load_ASettingsFileThatChangedOnDiskIsMergedAgain)
    {
        WriteSettingsFile(1);
        CreateSettings();
        EXPECT_EQ(Marker(), 1);

        WriteSettingsFile(33); // another size too, so the stamp differs whatever the clock resolution
        CreateSettings();
        EXPECT_EQ(Marker(), 33);
    }

    TEST_F(ExpanderSettingsDpeTests, Load_ANewRegistryMergesTheSettingsFileAgain)
    {
        WriteSettingsFile(1);
        CreateSettings();

        // the same file, a new registry (an application started again in the same process): it holds none of the file
        DestroyRegistry();
        CreateRegistry();
        EXPECT_EQ(Marker(), -1);
        CreateSettings();
        EXPECT_EQ(Marker(), 1);
    }

    TEST_F(ExpanderSettingsDpeTests, Load_ASettingsFileCreatedLaterIsMerged)
    {
        CreateSettings(); // no file yet
        EXPECT_EQ(Marker(), -1);

        WriteSettingsFile(4);
        CreateSettings();
        EXPECT_EQ(Marker(), 4);
    }
} // namespace AZ::DocumentPropertyEditor::Tests
