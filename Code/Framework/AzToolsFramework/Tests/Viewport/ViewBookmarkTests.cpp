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

namespace UnitTest
{
    struct LocalPersistentSettingsRegistry
    {
        AZStd::vector<char> m_buffer;
    };

    // fixture for testing the view bookmark save and load functionality
    class ViewBookmarkTestFixture : public PrefabTestFixture
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            PrefabTestFixture::SetUpEditorFixtureImpl();

            m_oldSettingsRegistry = AZ::SettingsRegistry::Get();
            if (m_oldSettingsRegistry)
            {
                AZ::SettingsRegistry::Unregister(m_oldSettingsRegistry);
            }

            m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            AZ::SettingsRegistry::Register(m_settingsRegistry.get());

            AZ::Entity* entity = nullptr;
            m_rootEntityId = UnitTest::CreateDefaultEditorEntity("Root", &entity);

            AzToolsFramework::ViewBookmarkPersistInterface* bookmarkPersistInterface =
                AZ::Interface<AzToolsFramework::ViewBookmarkPersistInterface>::Get();

            auto persistentSetReg = AZStd::make_shared<LocalPersistentSettingsRegistry>();
            bookmarkPersistInterface->OverrideStreamWriteFn(
                [persistentSetReg](
                    const AZ::IO::PathView&, AZStd::string_view stringBuffer,
                    AZStd::function<bool(AZ::IO::GenericStream&, const AZStd::string&)> write)
                {
                    persistentSetReg->m_buffer.resize(stringBuffer.size() + 1);

                    AZ::IO::MemoryStream memoryStream(persistentSetReg->m_buffer.data(), persistentSetReg->m_buffer.size(), 0);
                    const bool saved = write(memoryStream, stringBuffer);

                    EXPECT_THAT(saved, ::testing::IsTrue());

                    return saved;
                });

            bookmarkPersistInterface->OverrideStreamReadFn(
                [persistentSetReg](const AZ::IO::PathView&)
                {
                    return persistentSetReg->m_buffer;
                });

            bookmarkPersistInterface->OverrideFileExistsFn(
                [exists = false](const AZ::IO::PathView&) mutable
                {
                    // initially does not exist and is then created
                    return AZStd::exchange(exists, true);
                });
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
        using ::testing::IsFalse;
        AzToolsFramework::ViewBookmarkInterface* bookmarkInterface = AZ::Interface<AzToolsFramework::ViewBookmarkInterface>::Get();
        const AZStd::optional<AzToolsFramework::ViewBookmark> lastKnownLocationBookmark = bookmarkInterface->LoadLastKnownLocation();
        EXPECT_THAT(lastKnownLocationBookmark.has_value(), IsFalse());
    }

    TEST_F(ViewBookmarkTestFixture, ViewBookmarkLastKnownLocationCanBeStoredAndRetrieved)
    {
        using ::testing::IsFalse;
        AzToolsFramework::ViewBookmarkInterface* bookmarkInterface = AZ::Interface<AzToolsFramework::ViewBookmarkInterface>::Get();

        const auto cameraPosition = AZ::Vector3(0.0f, 20.0f, 12.0f);
        const auto expectedCameraRotationXDegrees = -35.0f;
        const auto expectedCameraRotationZDegrees = 90.0f;

        const AzFramework::CameraState cameraState = AzFramework::CreateDefaultCamera(
            AZ::Transform::CreateFromMatrix3x3AndTranslation(
                AZ::Matrix3x3::CreateRotationZ(AZ::DegToRad(expectedCameraRotationZDegrees)) *
                    AZ::Matrix3x3::CreateRotationX(AZ::DegToRad(expectedCameraRotationXDegrees)),
                cameraPosition),
            AzFramework::ScreenSize(1280, 720));

        AzToolsFramework::StoreViewBookmarkLastKnownLocationFromCameraState(cameraState);

        const AZStd::optional<AzToolsFramework::ViewBookmark> lastKnownLocationBookmark = bookmarkInterface->LoadLastKnownLocation();

        EXPECT_THAT(lastKnownLocationBookmark->m_position, IsClose(cameraPosition));
        EXPECT_THAT(
            lastKnownLocationBookmark->m_rotation,
            IsClose(AZ::Vector3(expectedCameraRotationXDegrees, 0.0f, expectedCameraRotationZDegrees)));
    }

    TEST_F(ViewBookmarkTestFixture, ViewBookmarkCanBeStoredAndRetrievedAtIndex)
    {
        using ::testing::IsFalse;
        AzToolsFramework::ViewBookmarkInterface* bookmarkInterface = AZ::Interface<AzToolsFramework::ViewBookmarkInterface>::Get();

        const auto index = 4;
        const auto cameraPosition = AZ::Vector3(13.0f, 20.0f, 70.0f);
        const auto expectedCameraRotationXDegrees = 75.0f;
        const auto expectedCameraRotationZDegrees = 64.0f;

        const AzFramework::CameraState cameraState = AzFramework::CreateDefaultCamera(
            AZ::Transform::CreateFromMatrix3x3AndTranslation(
                AZ::Matrix3x3::CreateRotationZ(AZ::DegToRad(expectedCameraRotationZDegrees)) *
                    AZ::Matrix3x3::CreateRotationX(AZ::DegToRad(expectedCameraRotationXDegrees)),
                cameraPosition),
            AzFramework::ScreenSize(1280, 720));

        AzToolsFramework::StoreViewBookmarkFromCameraStateAtIndex(index, cameraState);

        const AZStd::optional<AzToolsFramework::ViewBookmark> lastKnownLocationBookmark = bookmarkInterface->LoadBookmarkAtIndex(4);

        #if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        // Expected:  (X: 75,      Y:  0, Z: 64)
        // Actual:    (X: 74.9989, Y: -0, Z: 64)
        // Delta:          0.0011      0,     0
        constexpr float cameraPositionTolerance = 0.0012f;
        EXPECT_THAT(lastKnownLocationBookmark->m_position, IsCloseTolerance(cameraPosition, cameraPositionTolerance));
        #else
        EXPECT_THAT(lastKnownLocationBookmark->m_position, IsClose(cameraPosition));
        #endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON

        #if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        // Expected:  (X: 75,      Y: 0,  Z: 64)
        // Actual:    (X: 74.9989, Y: -0, Z: 64)
        // Delta:          0.0011      0,     0
        constexpr float cameraRotationTolerance = 0.0012f;
        EXPECT_THAT(
            lastKnownLocationBookmark->m_rotation,
            IsCloseTolerance(AZ::Vector3(expectedCameraRotationXDegrees, 0.0f, expectedCameraRotationZDegrees), cameraRotationTolerance));
        #else
        EXPECT_THAT(
            lastKnownLocationBookmark->m_rotation,
            IsClose(AZ::Vector3(expectedCameraRotationXDegrees, 0.0f, expectedCameraRotationZDegrees)));
        #endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON

    }
} // namespace UnitTest
