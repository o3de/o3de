/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Settings/ConfigParser.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/containers/fixed_unordered_map.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    struct ConfigFileParams
    {
        AZStd::string_view m_testConfigFileName;
        AZStd::string_view m_testConfigContents;
        using SettingsValueVariant = AZStd::variant<AZ::s64, bool, double, AZStd::string_view>;
        struct SettingsKeyValueTuple
        {
            AZStd::string_view m_key;
            SettingsValueVariant m_value;
            AZStd::string_view m_sectionHeader;
        };
        // The following test below will not have more than 32 settings in their config files
        AZStd::fixed_vector<SettingsKeyValueTuple, 20> m_expectedSettings;
    };

    class ConfigParserTestFixture
        : public LeakDetectionFixture
    {
    protected:
        AZStd::string m_configBuffer;
        AZ::IO::ByteContainerStream<AZStd::string> m_configStream{ &m_configBuffer };
    };

    // Parameterized test fixture for the ConfigFile Params
    class ConfigParserParamFixture
        : public ConfigParserTestFixture
        , public ::testing::WithParamInterface<ConfigFileParams>
    {
        void SetUp() override
        {
            auto configFileParam = GetParam();

            // Create the test config stream
            m_configStream.Write(configFileParam.m_testConfigContents.size(),
                configFileParam.m_testConfigContents.data());
            // Seek back to the beginning of the stream for test to read written data
            m_configStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        }
    };

    TEST_F(ConfigParserTestFixture, ParseConfigFile_WithEmptyFunction_Fails)
    {
        AZ::Settings::ConfigParserSettings parserSettings{ AZ::Settings::ConfigParserSettings::ParseConfigEntryFunc{} };

        m_configBuffer = R"(project_path=/TestProject
            engine_path=/TestEngine
        )";
        EXPECT_FALSE(AZ::Settings::ParseConfigFile(m_configStream, parserSettings));
    }

    TEST_F(ConfigParserTestFixture, ParseConfigFile_WithParseConfigEntryFunctionWhichAlwaysReturnsFalse_Fails)
    {
        auto parseConfigEntry = [](const AZ::Settings::ConfigParserSettings::ConfigEntry&)
        {
            return false;
        };

        AZ::Settings::ConfigParserSettings parserSettings{ parseConfigEntry };

        m_configBuffer = R"(project_path=/TestProject
            engine_path=/TestEngine
        )";
        EXPECT_FALSE(AZ::Settings::ParseConfigFile(m_configStream, parserSettings));

    }

    TEST_F(ConfigParserTestFixture, ParseConfigFile_WithLineLargerThan4096_Fails)
    {
        auto parseConfigEntry = [](const AZ::Settings::ConfigParserSettings::ConfigEntry&)
        {
            return true;
        };

        AZ::Settings::ConfigParserSettings parserSettings{ parseConfigEntry };

        m_configBuffer = "project_path=/TestProject\n";

        // append a line longer than 4096 characters
        m_configBuffer += "foo=";
        m_configBuffer.append(4096, 'a');
        m_configBuffer += '\n';

        EXPECT_FALSE(AZ::Settings::ParseConfigFile(m_configStream, parserSettings));
    }

    TEST_F(ConfigParserTestFixture, ParseConfigFile_WithAllLinesSmallerThan4097_Succeeds)
    {
        auto parseConfigEntry = [](const AZ::Settings::ConfigParserSettings::ConfigEntry&)
        {
            return true;
        };

        AZ::Settings::ConfigParserSettings parserSettings{ parseConfigEntry };

        m_configBuffer = "project_path=/TestProject\n";

        // append only 4000 characters
        m_configBuffer += "foo=";
        m_configBuffer.append(4000, 'a');
        m_configBuffer += '\n';

        EXPECT_TRUE(AZ::Settings::ParseConfigFile(m_configStream, parserSettings));
    }

    TEST_P(ConfigParserParamFixture, ParseConfigFile_ParseContents_Successfully)
    {
        auto configFileParam = GetParam();

        // Parse Config File and write output to a map for testing
        struct TestConfigEntry
        {
            AZStd::fixed_string<128> m_value;
            AZStd::fixed_string<128> m_sectionHeader;
        };
        using ParseSettingsMap = AZStd::fixed_unordered_map<AZStd::fixed_string<128>, TestConfigEntry, 7, 32>;
        ParseSettingsMap parseSettingsMap;
        auto parseConfigEntry = [&parseSettingsMap](const AZ::Settings::ConfigParserSettings::ConfigEntry& configEntry)
        {
            AZStd::fixed_string<128> fullKey(configEntry.m_sectionHeader);
            if (!fullKey.empty())
            {
                fullKey += '/';
            }
            fullKey += configEntry.m_keyValuePair.m_key;

            parseSettingsMap.emplace(fullKey, TestConfigEntry{
                AZStd::fixed_string<128>(configEntry.m_keyValuePair.m_value),
                AZStd::fixed_string<128>(configEntry.m_sectionHeader) });
            return true;
        };

        AZ::Settings::ConfigParserSettings parserSettings{ parseConfigEntry };

        // Update the comment prefix function to also support `--` as a comment chaacter
        parserSettings.m_commentPrefixFunc = [](AZStd::string_view line) -> AZStd::string_view
        {
            constexpr AZStd::string_view commentPrefixes[]{ "--", ";","#" };
            for (AZStd::string_view commentPrefix : commentPrefixes)
            {
                if (size_t commentOffset = line.find(commentPrefix); commentOffset != AZStd::string_view::npos)
                {
                    line = line.substr(0, commentOffset);
                }
            }
            return line;
        };

        auto parseOutcome = AZ::Settings::ParseConfigFile(m_configStream, parserSettings);
        EXPECT_TRUE(parseOutcome);

        // Validate that map contains expected settings
        for (auto&& expectedSettingPair : configFileParam.m_expectedSettings)
        {
            auto ValidateExpectedSettings = [&parseSettingsMap, settingsKey = expectedSettingPair.m_key,
                sectionHeader = expectedSettingPair.m_sectionHeader](auto&& settingsValue)
            {
                auto foundIt = parseSettingsMap.find(AZStd::fixed_string<128>(settingsKey));
                ASSERT_NE(parseSettingsMap.end(), foundIt);

                // Check the section header
                EXPECT_EQ(sectionHeader, foundIt->second.m_sectionHeader);

                // Check the value
                using SettingsValueType = AZStd::remove_cvref_t<decltype(settingsValue)>;
                if constexpr (AZStd::is_same_v<SettingsValueType, AZ::s64>
                    || AZStd::is_same_v<SettingsValueType, AZ::u64>
                    || AZStd::is_same_v<SettingsValueType, double>
                    || AZStd::is_same_v<SettingsValueType, bool>)
                {
                    AZStd::fixed_string<32> settingsValueAsString;
                    AZStd::to_string(settingsValueAsString, settingsValue);
                    EXPECT_EQ(AZStd::string_view(settingsValueAsString), foundIt->second.m_value);
                }
                else if constexpr (AZStd::is_same_v<SettingsValueType, AZStd::string_view>)
                {
                    EXPECT_EQ(settingsValue, foundIt->second.m_value);
                }
            };
            AZStd::visit(ValidateExpectedSettings, expectedSettingPair.m_value);
        }
    }

INSTANTIATE_TEST_CASE_P(
    ReadConfigFile,
    ConfigParserParamFixture,
    ::testing::Values(
        // Processes a fake bootstrap.cfg file which contains no section headers
        // and properly terminates the file with a newline
        ConfigFileParams{ "fake_bootstrap.cfg", R"(
-- When you see an option that does not have a platform preceding it, that is the default
-- value for anything not specifically set per platform. So if remote_filesystem=0 and you have
-- ios_remote_file_system=1 then remote filesystem will be off for all platforms except ios
-- Any of the settings in this file can be prefixed with a platform name:
-- android, ios, mac, linux, windows, etc...
-- or left unprefixed, to set all platforms not specified. The rules apply in the order they're declared

project_path=TestProject

-- remote_filesystem - enable Virtual File System (VFS)
-- This feature allows a remote instance of the game to run off assets
-- on the asset processor computers cache instead of deploying them the remote device
-- By default it is off and can be overridden for any platform
remote_filesystem=0
android_remote_filesystem=0
ios_remote_filesystem=0
mac_remote_filesystem=0

-- What type of assets are we going to load?
-- We need to know this before we establish VFS because different platform assets
-- are stored in different root folders in the cache.  These correspond to the names
-- In the asset processor config file.  This value also controls what config file is read
-- when you read system_xxxx_xxxx.cfg (for example, system_windows_pc.cfg or system_android_android.cfg)
-- by default, pc assets (in the 'pc' folder) are used, with RC being fed 'pc' as the platform
-- by default on console we use the default assets=pc for better iteration times
-- we should turn on console specific assets only when in release and/or testing assets and/or loading performance
-- that way most people will not need to have 3 different caches taking up disk space
assets = pc
android_assets = android
ios_assets = ios
mac_assets = mac

-- Add the IP address of your console to the white list that will connect to the asset processor here
-- You can list addresses or CIDR's. CIDR's are helpful if you are using DHCP. A CIDR looks like an ip address with
-- a /n on the end means how many bits are significant. 8bits.8bits.8bits.8bits = /32
-- Example: 192.168.1.3
-- Example: 192.168.1.3, 192.168.1.15
-- Example: 192.168.1.0/24 will allow any address starting with 192.168.1.
-- Example: 192.168.0.0/16 will allow any address starting with 192.168.
-- Example: 192.168.0.0/8 will allow any address starting with 192.
-- allowed_list =

-- IP address and optionally port of the asset processor.
-- Set your PC IP here: (and uncomment the next line)
-- If you are running your asset processor on a windows machine you
-- can find out your ip address by opening a cmd prompt and typing in ipconfig
-- remote_ip = 127.0.0.1
-- remote_port = 45643

-- Which way do you want to connect the asset processor to the game: 1=game connects to AP "connect", 0=AP connects to game "listen"
-- Note: android and IOS over USB port forwarding may need to listen instead of connect
connect_to_remote=0
windows_connect_to_remote=1
android_connect_to_remote=0
ios_connect_to_remote=0
mac_connect_to_remote=0

-- Should we tell the game to wait and not proceed unless we have a connection to the AP or
-- do we allow it to continue to try to connect in the background without waiting
-- Note: Certain options REQUIRE that we do not proceed unless we have a connection, and will override this option to 1 when set
-- Since remote_filesystem=1 requires a connection to proceed it will override our option to 1
wait_for_connect=0
windows_wait_for_connect=1
android_wait_for_connect=0
ios_wait_for_connect=0
mac_wait_for_connect=0

-- How long applications should wait while attempting to connect to an already launched AP(in seconds)
-- connect_ap_timeout=3

-- How long application should wait when launching the AP and wait for the AP to connect back to it(in seconds)
-- This time is dependent on Machine load as well as how long it takes for the new AP instance to initialize
-- A debug AP takes longer to start up than a profile AP
-- launch_ap_timeout=15

-- How long to wait for the AssetProcessor to be ready(i.e have all critical assets processed)
-- wait_ap_ready_timeout = 1200
# Commented out line using a number sign character
; Commented out line using a semicolon

)"
        , AZStd::fixed_vector<ConfigFileParams::SettingsKeyValueTuple, 20>{
            ConfigFileParams::SettingsKeyValueTuple{"project_path", AZStd::string_view{"TestProject"}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"remote_filesystem", AZ::s64{0}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"android_remote_filesystem", AZ::s64{0}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"ios_remote_filesystem", AZ::s64{0}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"mac_remote_filesystem", AZ::s64{0}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"assets", AZStd::string_view{"pc"}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"android_assets", AZStd::string_view{"android"}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"ios_assets", AZStd::string_view{"ios"}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"mac_assets", AZStd::string_view{"mac"}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"connect_to_remote", AZ::s64{0}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"windows_connect_to_remote", AZ::s64{1}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"android_connect_to_remote", AZ::s64{0}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"ios_connect_to_remote", AZ::s64{0}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"mac_connect_to_remote", AZ::s64{0}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"wait_for_connect", AZ::s64{0}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"windows_wait_for_connect", AZ::s64{1}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"android_wait_for_connect", AZ::s64{0}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"ios_wait_for_connect", AZ::s64{0}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"mac_wait_for_connect", AZ::s64{0}, {}},
        }},
        // Parses a fake AssetProcessorPlatformConfig file which contains sections headers
        // and does not end with a newline
            ConfigFileParams{ "fake_AssetProcessorPlatformConfig.ini", R"(
; ---- Enable/Disable platforms for the entire project. AssetProcessor will automatically add the current platform by default.

; PLATFORM DEFINITIONS
; [Platform (unique identifier)]
; tags=(comma-seperated-tags)
;
; note:  the 'identifier' of a platform is the word(s) following the "Platform" keyword (so [Platform pc] means identifier
;        is 'pc' for example.  This is used to name its assets folder in the cache and should be used in your bootstrap.cfg
;        or your main.cpp to choose what assets to load for that particular platform.
;        Its primary use is to enable additional non-host platforms (Ios, android...) that are not the current platform.
; note:  'tags' is a comma-seperated list of tags to tag the platform with that builders can inspect to decide what to do.

; while builders can accept any tags you add in order to make decisions, common tags are
; tools - this platform can host the tools and editor and such
; renderer - this platform runs the client engine and renders on a GPU.  If missing we could be on a server-only platform
; mobile - a mobile platform such as a set top box or phone with limited resources
; console - a console platform
; server - a server platform of some kind, usually headless, no renderer.

test_asset_processor_tag = test_value
[Platform pc]
tags=tools,renderer,dx12,vulkan

[Platform android]
tags=android,mobile,renderer,vulkan ; With Comments at the end
; validate leading and trailing whitespace is not parsed when examining the section header
      [Platform ios]     
tags=mobile,renderer,metal

[Platform mac]
tags=tools,renderer,metal)"
        , AZStd::fixed_vector<ConfigFileParams::SettingsKeyValueTuple, 20>{
            ConfigFileParams::SettingsKeyValueTuple{"test_asset_processor_tag", AZStd::string_view{"test_value"}, {}},
            ConfigFileParams::SettingsKeyValueTuple{"Platform pc/tags", AZStd::string_view{"tools,renderer,dx12,vulkan"}, "Platform pc"},
            ConfigFileParams::SettingsKeyValueTuple{"Platform android/tags", AZStd::string_view{"android,mobile,renderer,vulkan"}, "Platform android"},
            ConfigFileParams::SettingsKeyValueTuple{"Platform ios/tags", AZStd::string_view{"mobile,renderer,metal"}, "Platform ios"},
            ConfigFileParams::SettingsKeyValueTuple{"Platform mac/tags", AZStd::string_view{"tools,renderer,metal"}, "Platform mac"},
        }}
        )
    );


}
