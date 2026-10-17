/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <SentryCrashCore/SentryCrashCore.h>

#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace
{
    //! LoadConfig reads through AZ::SettingsRegistry::Get(), so a test registry has to be
    //! registered for the duration of each case that exercises overrides.
    class ScopedTestRegistry
    {
    public:
        ScopedTestRegistry()
        {
            // Save and restore any pre-existing global registry, matching the pattern in
            // AzCore's own SettingsRegistry tests.
            m_previous = AZ::SettingsRegistry::Get();
            if (m_previous != nullptr)
            {
                AZ::SettingsRegistry::Unregister(m_previous);
            }
            AZ::SettingsRegistry::Register(&m_registry);
        }
        ~ScopedTestRegistry()
        {
            AZ::SettingsRegistry::Unregister(&m_registry);
            if (m_previous != nullptr)
            {
                AZ::SettingsRegistry::Register(m_previous);
            }
        }
        AZ::SettingsRegistryImpl& Get()
        {
            return m_registry;
        }

    private:
        AZ::SettingsRegistryImpl m_registry;
        AZ::SettingsRegistryInterface* m_previous{};
    };
}

using SentryCrashCoreFixture = UnitTest::LeakDetectionFixture;

TEST_F(SentryCrashCoreFixture, LoadConfig_DerivesDatabasePathUnderAppRoot)
{
    ScopedTestRegistry registry;

    SentryCrash::Config config = SentryCrash::LoadConfig("Editor", "C:/projects/Blank_Testing");

    EXPECT_STREQ(config.m_databasePath.c_str(), "C:/projects/Blank_Testing/.sentry-native");
    EXPECT_STREQ(config.m_moduleTag.c_str(), "Editor");
}

TEST_F(SentryCrashCoreFixture, LoadConfig_DoesNotDoubleSeparatorWhenAppRootHasTrailingSlash)
{
    ScopedTestRegistry registry;

    SentryCrash::Config config = SentryCrash::LoadConfig("Editor", "C:/projects/Blank_Testing/");

    EXPECT_STREQ(config.m_databasePath.c_str(), "C:/projects/Blank_Testing/.sentry-native");
}

TEST_F(SentryCrashCoreFixture, LoadConfig_DefaultsAreSafeWhenRegistryIsEmpty)
{
    ScopedTestRegistry registry;

    SentryCrash::Config config = SentryCrash::LoadConfig("Editor", "C:/projects/X");

    // An absent DSN must leave crash reporting off rather than half-configured.
    EXPECT_TRUE(config.m_dsn.empty());
    EXPECT_STREQ(config.m_environment.c_str(), "development");
    EXPECT_FALSE(config.m_sendDefaultPii);
    EXPECT_TRUE(config.m_scrubUserPaths);
}

TEST_F(SentryCrashCoreFixture, LoadConfig_ReadsOverridesFromSettingsRegistry)
{
    ScopedTestRegistry registry;
    registry.Get().Set("/O3DE/CrashReporting/SentryDsn", "https://key@example.com/42");
    registry.Get().Set("/O3DE/CrashReporting/Environment", "production");
    registry.Get().Set("/O3DE/CrashReporting/SampleRate", 0.25);
    registry.Get().Set("/O3DE/CrashReporting/AttachScreenshot", false);
    registry.Get().Set("/O3DE/CrashReporting/MaxBreadcrumbs", AZ::s64{ 25 });
    registry.Get().Set("/O3DE/CrashReporting/AppHangTimeoutMs", AZ::s64{ 9000 });

    SentryCrash::Config config = SentryCrash::LoadConfig("GameLauncher", "C:/projects/X");

    EXPECT_STREQ(config.m_dsn.c_str(), "https://key@example.com/42");
    EXPECT_STREQ(config.m_environment.c_str(), "production");
    EXPECT_DOUBLE_EQ(config.m_sampleRate, 0.25);
    EXPECT_FALSE(config.m_attachScreenshot);
    EXPECT_EQ(config.m_maxBreadcrumbs, 25u);
    EXPECT_EQ(config.m_appHangTimeoutMs, 9000u);
}

TEST_F(SentryCrashCoreFixture, ContextHelpers_AreNoOpsBeforeInitialize)
{
    // Every enrichment entry point must tolerate being called when the DSN was absent and
    // Initialize() bailed out, since callers are not expected to branch on that.
    EXPECT_FALSE(SentryCrash::IsInitialized());
    SentryCrash::SetTag("level", "TestLevel");
    SentryCrash::SetLevelName("TestLevel");
    SentryCrash::AddBreadcrumb("default", "editor", "opened level");
    SentryCrash::SetContext("gpu", { { "name", "Test GPU" } });
    SentryCrash::AppHangHeartbeat();
    SentryCrash::Shutdown();
    EXPECT_FALSE(SentryCrash::IsInitialized());
}
