/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Viewport/LocalViewBookmarkComponent.h>
#include <AzToolsFramework/Viewport/LocalViewBookmarkLoader.h>

#include <Tests/Prefab/PrefabTestFixture.h>

#pragma optimize("", off)
#pragma inline_depth(0)

namespace UnitTest
{
    // Fixture for testing the viewport editor mode state tracker
    class ViewBookmarkTestFixture : public PrefabTestFixture
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            PrefabTestFixture::SetUpEditorFixtureImpl();

            CreateRootPrefab();

            m_oldSettingsRegistry = AZ::SettingsRegistry::Get();
            if (m_oldSettingsRegistry)
            {
                AZ::SettingsRegistry::Unregister(m_oldSettingsRegistry);
            }

            m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            AZ::SettingsRegistry::Register(m_settingsRegistry.get());

            AZ::Entity* entity = nullptr;
            m_rootEntityId = UnitTest::CreateDefaultEditorEntity("Root", &entity);
        }
        void TearDownEditorFixtureImpl() override
        {
            AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());
            if (m_oldSettingsRegistry)
            {
                AZ::SettingsRegistry::Register(m_oldSettingsRegistry);
                m_oldSettingsRegistry = nullptr;
            }

            m_settingsRegistry.reset();

            PrefabTestFixture::TearDownEditorFixtureImpl();
        }

        AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;
        AZ::SettingsRegistryInterface* m_oldSettingsRegistry = nullptr;
        AZ::EntityId m_rootEntityId;
    };

    TEST_F(ViewBookmarkTestFixture, ViewBookmarkInterfaceIsInstantiatedAsPartOfToolsApplication)
    {
        using ::testing::NotNull;
        AzToolsFramework::ViewBookmarkInterface* bookmarkInterface = AZ::Interface<AzToolsFramework::ViewBookmarkInterface>::Get();
        EXPECT_THAT(bookmarkInterface, NotNull());
    }

    TEST_F(ViewBookmarkTestFixture, ViewBookmarkLastKnownLocationIsNotFoundWithNoLevel)
    {
        AzToolsFramework::ViewBookmarkPersistInterface* bookmarkPersistInterface =
            AZ::Interface<AzToolsFramework::ViewBookmarkPersistInterface>::Get();

        struct LocalPersistentSettingsRegistry
        {
            AZStd::vector<char> m_buffer;
        };

        auto persistentSetReg = AZStd::make_shared<LocalPersistentSettingsRegistry>();
        bookmarkPersistInterface->SetStreamWriteFn(
            [persistentSetReg](
                [[maybe_unused]] const AZStd::string& localBookmarksFileName, const AZStd::string& stringBuffer,
                AZStd::function<bool(AZ::IO::GenericStream&, const AZStd::string&)> write)
            {
                persistentSetReg->m_buffer.resize(stringBuffer.size() + 1);

                AZ::IO::MemoryStream memoryStream(persistentSetReg->m_buffer.data(), persistentSetReg->m_buffer.size(), 0);
                const bool saved = write(memoryStream, stringBuffer);

                EXPECT_THAT(saved, ::testing::IsTrue());

                return saved;
            });

        bookmarkPersistInterface->SetStreamReadFn(
            [persistentSetReg]([[maybe_unused]] const AZStd::string& localBookmarksFileName)
            {
                [[maybe_unused]] auto debugString = AZStd::string(persistentSetReg->m_buffer);
                return persistentSetReg->m_buffer;
            });

        bool exists = false;
        bookmarkPersistInterface->SetFileExistsFn(
            [&exists](const AZStd::string&)
            {
                return AZStd::exchange(exists, true);
            });

        using ::testing::IsFalse;
        AzToolsFramework::ViewBookmarkInterface* bookmarkInterface = AZ::Interface<AzToolsFramework::ViewBookmarkInterface>::Get();
        const AZStd::optional<AzToolsFramework::ViewBookmark> lastKnownLocationBookmark = bookmarkInterface->LoadLastKnownLocation();
        EXPECT_THAT(lastKnownLocationBookmark.has_value(), IsFalse());
    }
} // namespace UnitTest

#pragma optimize("", on)
#pragma inline_depth()
