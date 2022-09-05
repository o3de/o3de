/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/CommandLine.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Utils/Utils.h>

namespace SettingsRegistryMergeUtilsTests
{
    struct DumpSettingsRegistryParams
    {
        AZ::SettingsRegistryInterface::Format m_jsonFormat{ AZ::SettingsRegistryInterface::Format::JsonMergePatch };
        const char* m_inputJsonDocument{ "" };
        const char* m_expectedDumpString{ "" };
        AZ::SettingsRegistryMergeUtils::DumperSettings m_dumperSettings;
        AZStd::string_view m_jsonPointerPath;
    };

    class SettingsRegistryMergeUtilsParamFixture
        : public UnitTest::ScopedAllocatorSetupFixture
        , public ::testing::WithParamInterface<DumpSettingsRegistryParams>
    {
    public:

        void SetUp() override
        {
            m_registry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
        }

        void TearDown() override
        {
            m_registry.reset();
        }

        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_registry;
    };

    TEST_P(SettingsRegistryMergeUtilsParamFixture, DumpSettingsToByteContainerStream_ReturnsExpected)
    {
        const DumpSettingsRegistryParams& param = GetParam();

        ASSERT_TRUE(m_registry->MergeSettings(param.m_inputJsonDocument, param.m_jsonFormat));

        AZStd::string dumpString;
        AZ::IO::ByteContainerStream stringStream(&dumpString);
        EXPECT_TRUE(AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(*m_registry, param.m_jsonPointerPath, stringStream,
            param.m_dumperSettings));
        EXPECT_FALSE(dumpString.empty());
        EXPECT_STREQ(param.m_expectedDumpString, dumpString.c_str());
    }

    TEST_P(SettingsRegistryMergeUtilsParamFixture, DumpSettingsStdout_ReturnsExpected)
    {
        const DumpSettingsRegistryParams& param = GetParam();

        ASSERT_TRUE(m_registry->MergeSettings(param.m_inputJsonDocument, param.m_jsonFormat));

        AZ::IO::StdoutStream stdoutStream;
        EXPECT_TRUE(AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(*m_registry, param.m_jsonPointerPath, stdoutStream,
            param.m_dumperSettings));
    }

    INSTANTIATE_TEST_CASE_P(
        DumpSettings,
        SettingsRegistryMergeUtilsParamFixture,
        ::testing::Values(
            DumpSettingsRegistryParams
            {
                AZ::SettingsRegistryInterface::Format::JsonPatch,
                R"([)" "\n"
                R"(    { "op": "add", "path": "/Test", "value": { "Object": {} } },)" "\n"
                R"(    { "op": "add", "path": "/Test/Object/NullType", "value": null },)" "\n"
                R"(    { "op": "add", "path": "/Test/Object/TrueType", "value": true },)" "\n"
                R"(    { "op": "add", "path": "/Test/Object/FalseType", "value": false },)" "\n"
                R"(    { "op": "add", "path": "/Test/Object/IntType", "value": -42 },)" "\n"
                R"(    { "op": "add", "path": "/Test/Object/UIntType", "value": 42 },)" "\n"
                R"(    { "op": "add", "path": "/Test/Object/DoubleType", "value": 42.0 },)" "\n"
                R"(    { "op": "add", "path": "/Test/Object/StringType", "value": "Hello world" },)" "\n"
                R"(    { "op": "add", "path": "/Test/Array", "value": [ null, true, false, -42, 42, 42.0, "Hello world" ] })" "\n"
                R"(])" "\n",
                R"({"Test":{"Object":{"NullType":null,"TrueType":true,"FalseType":false,"IntType":-42,"UIntType":42)"
                R"(,"DoubleType":42.0,"StringType":"Hello world"},"Array":[null,true,false,-42,42,42.0,"Hello world"]}})",
                AZ::SettingsRegistryMergeUtils::DumperSettings
                {
                    false,
                    [](AZStd::string_view path)
                    {
                        AZStd::string_view prefixPath("/Test");
                        return prefixPath.starts_with(path.substr(0, prefixPath.size()));
                    }
                }
            },
            DumpSettingsRegistryParams
            {
                AZ::SettingsRegistryInterface::Format::JsonMergePatch,
                R"({)" "\n"
                R"(    "Test":)" "\n"
                R"(    {)" "\n"
                R"(        "Array0": [ 142, 188 ], )" "\n"
                R"(        "Array1": [ 242, 288 ], )" "\n"
                R"(        "Array2": [ 342, 388 ] )" "\n"
                R"(    })" "\n"
                R"(})" "\n",
                R"({"Test":{"Array0":[142,188],"Array1":[242,288],"Array2":[342,388]}})",
                AZ::SettingsRegistryMergeUtils::DumperSettings{ false,
                    [](AZStd::string_view path)
                    {
                        AZStd::string_view prefixPath("/Test");
                        return prefixPath.starts_with(path.substr(0, prefixPath.size()));
                    }
                }
            },
            DumpSettingsRegistryParams
            {
                AZ::SettingsRegistryInterface::Format::JsonMergePatch,
                R"({
                    "Test":
                    {
                        "Array0": [ 142, 188 ],
                        "Array1": [ 242, 288 ],
                        "Array2": [ 342, 388 ]
                    }
                    })",
                R"({)""\n"
                R"(    "Test": {)""\n"
                R"(        "Array0": [)""\n"
                R"(            142,)""\n"
                R"(            188)""\n"
                R"(        ],)""\n"
                R"(        "Array1": [)""\n"
                R"(            242,)""\n"
                R"(            288)""\n"
                R"(        ],)""\n"
                R"(        "Array2": [)""\n"
                R"(            342,)""\n"
                R"(            388)""\n"
                R"(        ])""\n"
                R"(    })""\n"
                R"(})",
                AZ::SettingsRegistryMergeUtils::DumperSettings
                {
                    true,
                    [](AZStd::string_view path)
                    {
                        AZStd::string_view prefixPath("/Test");
                        return prefixPath.starts_with(path.substr(0, prefixPath.size()));
                    }
                }
            },
            DumpSettingsRegistryParams
            {
                AZ::SettingsRegistryInterface::Format::JsonMergePatch,
                R"({
                    "Test":
                    {
                        "Array0": [ 142, 188 ],
                        "Array1": [ 242, 288 ],
                        "Array2": [ 342, 388 ]
                    }
                })",
                R"({)""\n"
                R"(    "Array0": [)""\n"
                R"(        142,)""\n"
                R"(        188)""\n"
                R"(    ],)""\n"
                R"(    "Array1": [)""\n"
                R"(        242,)""\n"
                R"(        288)""\n"
                R"(    ],)""\n"
                R"(    "Array2": [)""\n"
                R"(        342,)""\n"
                R"(        388)""\n"
                R"(    ])""\n"
                R"(})",
                AZ::SettingsRegistryMergeUtils::DumperSettings
                {
                    true,
                    [](AZStd::string_view path)
                    {
                        AZStd::string_view prefixPath("/Test");
                        return prefixPath.starts_with(path.substr(0, prefixPath.size()));
                    }
                },
                "/Test"
            },
            DumpSettingsRegistryParams{
                AZ::SettingsRegistryInterface::Format::JsonMergePatch,
                R"({)" "\n"
                R"(    "Test":)" "\n"
                R"(    {)" "\n"
                R"(        "Array0": [ 142, 188 ])" "\n"
                R"(    })" "\n"
                R"(})",
                R"({"Root":{"Path":{"Test":{"Array0":[142,188]}}}})",
                AZ::SettingsRegistryMergeUtils::DumperSettings{ false, {}, "/Root/Path/Test" },
                "/Test"
            })
    );


    static bool CreateTestFile(const AZ::IO::FixedMaxPath& testPath, AZStd::string_view content)
    {
        AZ::IO::SystemFile file;
        if (!file.Open(testPath.c_str(), AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE
            | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
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

    //! ConfigFile MergeUtils Test
    struct ConfigFileParams
    {
        AZStd::string_view m_testConfigFileName;
        AZStd::string_view m_testConfigContents;
        using SettingsValueVariant = AZStd::variant<AZ::s64, bool, double, AZStd::string_view>;
        using SettingsKeyValuePair = AZStd::pair<AZStd::string_view, SettingsValueVariant>;
        // The following test below will not have more than 32 settings in their config files
        AZStd::fixed_vector<SettingsKeyValuePair, 20> m_expectedSettings;
    };
    class SettingsRegistryMergeUtilsConfigFileFixture
        : public UnitTest::ScopedAllocatorSetupFixture
        , public ::testing::WithParamInterface<ConfigFileParams>
    {
    public:

        void SetUp() override
        {
            m_registry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            auto configFileParam = GetParam();
            auto testPath = AZ::IO::FixedMaxPath(m_testFolder.GetDirectory()) / configFileParam.m_testConfigFileName;
            // Create the test config file
            ASSERT_TRUE(CreateTestFile(testPath, configFileParam.m_testConfigContents));
        }

        void TearDown() override
        {
            m_registry.reset();
        }

    protected:
        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_registry;
        AZ::Test::ScopedAutoTempDirectory m_testFolder;
    };

    TEST_P(SettingsRegistryMergeUtilsConfigFileFixture, MergeSettingsToRegistry_ConfigFile_ParseContents_Successfully)
    {
        auto configFileParam = GetParam();

        // Merge Config File to Settings Registry
        AZ::SettingsRegistryMergeUtils::ConfigParserSettings parserSettings;
        parserSettings.m_commentPrefixFunc = [](AZStd::string_view line) -> AZStd::string_view
        {
            constexpr AZStd::string_view commentPrefixes[]{ "--", ";","#" };
            for (AZStd::string_view commentPrefix : commentPrefixes)
            {
                if (size_t commentOffset = line.find(commentPrefix); commentOffset != AZStd::string_view::npos)
                {
                    return line.substr(0, commentOffset);
                }
            }
            return line;
        };

        auto testPath = AZ::IO::FixedMaxPath(m_testFolder.GetDirectory()) / configFileParam.m_testConfigFileName;
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ConfigFile(*m_registry, testPath.Native(), parserSettings);

        // Validate that Settings Registry contains expected settings
        for (auto&& expectedSettingPair : configFileParam.m_expectedSettings)
        {
            auto ValidateExpectedSettings = [this, settingsKey = expectedSettingPair.first](auto&& settingsValue)
            {
                using SettingsValueType = AZStd::remove_cvref_t<decltype(settingsValue)>;
                if constexpr (AZStd::is_same_v<SettingsValueType, AZ::s64>
                    || AZStd::is_same_v<SettingsValueType, double>
                    || AZStd::is_same_v<SettingsValueType, bool>)
                {
                    SettingsValueType registryValue{};
                    EXPECT_TRUE(m_registry->Get(registryValue, settingsKey));
                    EXPECT_EQ(settingsValue, registryValue);
                }
                else if constexpr (AZStd::is_same_v<SettingsValueType, AZStd::string_view>)
                {
                    AZ::SettingsRegistryInterface::FixedValueString registryValue;
                    EXPECT_TRUE(m_registry->Get(registryValue, settingsKey));
                    EXPECT_EQ(settingsValue, registryValue);
                }
            };
            AZStd::visit(ValidateExpectedSettings, expectedSettingPair.second);
        }
    }

INSTANTIATE_TEST_CASE_P(
    ReadConfigFile,
    SettingsRegistryMergeUtilsConfigFileFixture,
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
        , AZStd::fixed_vector<ConfigFileParams::SettingsKeyValuePair, 20>{
            ConfigFileParams::SettingsKeyValuePair{"/project_path", AZStd::string_view{"TestProject"}},
            ConfigFileParams::SettingsKeyValuePair{"/remote_filesystem", AZ::s64{0}},
            ConfigFileParams::SettingsKeyValuePair{"/android_remote_filesystem", AZ::s64{0}},
            ConfigFileParams::SettingsKeyValuePair{"/ios_remote_filesystem", AZ::s64{0}},
            ConfigFileParams::SettingsKeyValuePair{"/mac_remote_filesystem", AZ::s64{0}},
            ConfigFileParams::SettingsKeyValuePair{"/assets", AZStd::string_view{"pc"}},
            ConfigFileParams::SettingsKeyValuePair{"/android_assets", AZStd::string_view{"android"}},
            ConfigFileParams::SettingsKeyValuePair{"/ios_assets", AZStd::string_view{"ios"}},
            ConfigFileParams::SettingsKeyValuePair{"/mac_assets", AZStd::string_view{"mac"}},
            ConfigFileParams::SettingsKeyValuePair{"/connect_to_remote", AZ::s64{0}},
            ConfigFileParams::SettingsKeyValuePair{"/windows_connect_to_remote", AZ::s64{1}},
            ConfigFileParams::SettingsKeyValuePair{"/android_connect_to_remote", AZ::s64{0}},
            ConfigFileParams::SettingsKeyValuePair{"/ios_connect_to_remote", AZ::s64{0}},
            ConfigFileParams::SettingsKeyValuePair{"/mac_connect_to_remote", AZ::s64{0}},
            ConfigFileParams::SettingsKeyValuePair{"/wait_for_connect", AZ::s64{0}},
            ConfigFileParams::SettingsKeyValuePair{"/windows_wait_for_connect", AZ::s64{1}},
            ConfigFileParams::SettingsKeyValuePair{"/android_wait_for_connect", AZ::s64{0}},
            ConfigFileParams::SettingsKeyValuePair{"/ios_wait_for_connect", AZ::s64{0}},
            ConfigFileParams::SettingsKeyValuePair{"/mac_wait_for_connect", AZ::s64{0}},
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

[Platform ios]
tags=mobile,renderer,metal

[Platform mac]
tags=tools,renderer,metal)"
        , AZStd::fixed_vector<ConfigFileParams::SettingsKeyValuePair, 20>{
            ConfigFileParams::SettingsKeyValuePair{"/test_asset_processor_tag", AZStd::string_view{"test_value"}},
            ConfigFileParams::SettingsKeyValuePair{"/Platform pc/tags", AZStd::string_view{"tools,renderer,dx12,vulkan"}},
            ConfigFileParams::SettingsKeyValuePair{"/Platform android/tags", AZStd::string_view{"android,mobile,renderer,vulkan"}},
            ConfigFileParams::SettingsKeyValuePair{"/Platform ios/tags", AZStd::string_view{"mobile,renderer,metal"}},
            ConfigFileParams::SettingsKeyValuePair{"/Platform mac/tags", AZStd::string_view{"tools,renderer,metal"}},
        }}
        )
    );

    struct SettingsRegistryGemVisitParams
    {
        const char* m_o3deManifestJson{ "" };
        const char* m_engineManifestJson{ "" };
        const char* m_projectManifestJson{ "" };
        AZStd::fixed_vector<AZStd::tuple<const char*, const char*>, 8> m_gemManifestJsons;
        const char* m_activeGemJson{ "" };
        AZStd::fixed_vector<const char*, 8> m_expectedActiveGemPaths;
        AZStd::fixed_vector<const char*, 8> m_expectedManifestGemPaths;
    };

    class SettingsRegistryGemVisitFixture
        : public UnitTest::ScopedAllocatorSetupFixture
        , public ::testing::WithParamInterface<SettingsRegistryGemVisitParams>
    {
    public:
        void SetUp() override
        {
            m_registry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            // Create the manifest json files
            const auto& gemVisitParams = GetParam();
            auto tempRootFolder = AZ::IO::FixedMaxPath(m_testFolder.GetDirectory());
            AZ::IO::FixedMaxPath o3deManifestFilePath = tempRootFolder / "o3de" / "o3de_manifest.json";
            AZ::IO::FixedMaxPath engineManifestPath = tempRootFolder / "engine" / "engine.json";
            AZ::IO::FixedMaxPath projectManifestPath = tempRootFolder / "project" / "project.json";
            ASSERT_TRUE(CreateTestFile(o3deManifestFilePath, gemVisitParams.m_o3deManifestJson));
            ASSERT_TRUE(CreateTestFile(engineManifestPath, gemVisitParams.m_engineManifestJson));
            ASSERT_TRUE(CreateTestFile(projectManifestPath, gemVisitParams.m_projectManifestJson));

            for (const auto& [gemRelativePath, gemManifestJson] : gemVisitParams.m_gemManifestJsons)
            {
                AZ::IO::FixedMaxPath gemManifestPath = tempRootFolder / gemRelativePath / "gem.json";
                ASSERT_TRUE(CreateTestFile(gemManifestPath, gemManifestJson));
            }

            // Set the FilePathKeys for the o3de root, engine root and project root directories
            m_registry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_O3deManifestRootFolder,
                (tempRootFolder / "o3de").Native());
            m_registry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder,
                (tempRootFolder / "engine").Native());
            m_registry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath,
                (tempRootFolder / "project").Native());

            // Merge the Active Gem Json data to the Settings Registry
            EXPECT_TRUE(m_registry->MergeSettings(gemVisitParams.m_activeGemJson, AZ::SettingsRegistryInterface::Format::JsonMergePatch));
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ManifestGemsPaths(*m_registry);
        }

        void TearDown() override
        {
            m_registry.reset();
        }
    protected:
        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_registry;
        AZ::Test::ScopedAutoTempDirectory m_testFolder;
    };

    TEST_P(SettingsRegistryGemVisitFixture, SettingsRegistryMergeUtils_AllManifestGems_AreInSettingsRegistry)
    {
        AZStd::vector<AZ::IO::Path> manifestGemPaths;
        auto GetManifestGemPaths = [&manifestGemPaths, this](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            AZ::IO::Path gemPath;
            EXPECT_TRUE(m_registry->Get(gemPath.Native(), FixedValueString(visitArgs.m_jsonKeyPath) + "/Path"));
            manifestGemPaths.push_back(gemPath.LexicallyRelative(m_testFolder.GetDirectory()));
            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        };

        EXPECT_TRUE(AZ::SettingsRegistryVisitorUtils::VisitObject(*m_registry,
            GetManifestGemPaths, AZ::SettingsRegistryMergeUtils::ManifestGemsRootKey));
        const auto& gemVisitParams = GetParam();
        EXPECT_THAT(manifestGemPaths, ::testing::UnorderedElementsAreArray(
            gemVisitParams.m_expectedManifestGemPaths.begin(), gemVisitParams.m_expectedManifestGemPaths.end()));
    }

    TEST_P(SettingsRegistryGemVisitFixture, SettingsRegistryMergeUtils_VisitActiveGems_OnlyReturnsActiveGemPaths)
    {
        AZStd::vector<AZ::IO::Path> activeGemPaths;
        auto GetActiveGems = [&activeGemPaths, this](AZStd::string_view, AZStd::string_view gemPath)
        {
            activeGemPaths.push_back(AZ::IO::Path(gemPath).LexicallyRelative(m_testFolder.GetDirectory()));
        };
        AZ::SettingsRegistryMergeUtils::VisitActiveGems(*m_registry, GetActiveGems);

        const auto& gemVisitParams = GetParam();
        EXPECT_THAT(activeGemPaths, ::testing::UnorderedElementsAreArray(
            gemVisitParams.m_expectedActiveGemPaths.begin(), gemVisitParams.m_expectedActiveGemPaths.end()));
    }

    TEST_P(SettingsRegistryGemVisitFixture, Validate_SettingsRegistryUtilsQueries_WorksWithLocalRegistry)
    {
        const AZ::IO::FixedMaxPath tempRootFolder(m_testFolder.GetDirectory());
        AZ::IO::FixedMaxPath testPath = AZ::Utils::GetO3deManifestDirectory(m_registry.get());
        EXPECT_EQ((tempRootFolder / "o3de"), testPath);

        testPath = AZ::Utils::GetEnginePath(m_registry.get());
        EXPECT_EQ((tempRootFolder / "engine"), testPath);

        testPath = AZ::Utils::GetProjectPath(m_registry.get());
        EXPECT_EQ((tempRootFolder / "project"), testPath);

        testPath = AZ::Utils::GetGemPath("outerGem1", m_registry.get());
        EXPECT_EQ((tempRootFolder / "o3de/outerGem1"), testPath);

        testPath = AZ::Utils::GetGemPath("outerGem2", m_registry.get());
        EXPECT_EQ((tempRootFolder / "o3de/outerGem2"), testPath);

        testPath = AZ::Utils::GetGemPath("innerGem1", m_registry.get());
        EXPECT_EQ((tempRootFolder / "o3de/outerGem2/innerGem1"), testPath);

        testPath = AZ::Utils::GetGemPath("engineGem1", m_registry.get());
        EXPECT_EQ((tempRootFolder / "engine/engineGem1"), testPath);

        testPath = AZ::Utils::GetGemPath("projectGem1", m_registry.get());
        EXPECT_EQ((tempRootFolder / "project/projectGem1"), testPath);

        testPath = AZ::Utils::GetGemPath("outsideGem1", m_registry.get());
        EXPECT_EQ((tempRootFolder / "outsideGem1"), testPath);
    }

    static auto MakeGemVisitTestingValues()
    {
        return AZStd::array{
            SettingsRegistryGemVisitParams{
                R"({
                    "o3de_manifest_name": "testuser",
                    "external_subdirectories": [
                        "outerGem1",
                        "outerGem2"
                    ]
                   })",
                R"({
                    "engine_name": "o3de",
                    "external_subdirectories": [
                        "engineGem1"
                    ]
                   })",
                R"({
                    "project_name": "TestProject",
                    "external_subdirectories": [
                        "projectGem1",
                        "../outsideGem1"
                    ]
                   })",
                {
                AZStd::make_tuple("o3de/outerGem1",
                    R"({
                            "gem_name": "outerGem1",
                       })"
                    ),
                AZStd::make_tuple("o3de/outerGem2",
                    R"({
                        "gem_name": "outerGem2",
                        "external_subdirectories": [
                            "innerGem1"
                        ]
                       })"
                    ),
                AZStd::make_tuple("o3de/outerGem2/innerGem1",
                    R"({
                        "gem_name": "innerGem1",
                       })"
                    ),
                AZStd::make_tuple("engine/engineGem1",
                    R"({
                        "gem_name": "engineGem1",
                       })"
                   ),
                AZStd::make_tuple("project/projectGem1",
                    R"({
                        "gem_name": "projectGem1",
                       })"
                    ),
                    AZStd::make_tuple("outsideGem1",
                    R"({
                        "gem_name": "outsideGem1",
                       })"
                    ),
                },
                R"({
                    "O3DE": {
                        "Gems": {
                            "outerGem1" : {},
                            "innerGem1" : {},
                            "engineGem1" : {},
                            "projectGem1" : {},
                        }
                    }
                   })",
                {
                    "o3de/outerGem1",
                    "o3de/outerGem2/innerGem1",
                    "engine/engineGem1",
                    "project/projectGem1"
                },
                {
                    "o3de/outerGem1",
                    "o3de/outerGem2",
                    "o3de/outerGem2/innerGem1",
                    "engine/engineGem1",
                    "project/projectGem1",
                    "outsideGem1"
                }
            } };
    }

    INSTANTIATE_TEST_CASE_P(
        MergeManifestJson,
        SettingsRegistryGemVisitFixture,
        ::testing::ValuesIn(MakeGemVisitTestingValues()));

    class SettingsRegistryMergeUtilsCommandLineFixture
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    public:
        void SetUp() override
        {
            m_registry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
        }

        void TearDown() override
        {
            m_registry.reset();
        }

        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_registry;
    };

    TEST_F(SettingsRegistryMergeUtilsCommandLineFixture, CommandLineArguments_MergeToSettingsRegistry_Success)
    {
        AZ::CommandLine commandLine;
        commandLine.Parse({ "programname.exe", "--project-path", "--RemoteIp", "10.0.0.1", "--ScanFolders", R"(\a\b\c,\d\e\f)", "Foo", "Bat" });

        AZ::SettingsRegistryMergeUtils::StoreCommandLineToRegistry(*m_registry, commandLine);
        // Clear the CommandLine instance
        commandLine = {};

        EXPECT_TRUE(AZ::SettingsRegistryMergeUtils::GetCommandLineFromRegistry(*m_registry, commandLine));

        ASSERT_TRUE(commandLine.HasSwitch("project-path"));
        EXPECT_EQ(1, commandLine.GetNumSwitchValues("project-path"));
        EXPECT_STREQ("", commandLine.GetSwitchValue("project-path", 0).c_str());

        ASSERT_TRUE(commandLine.HasSwitch("remoteip"));
        ASSERT_EQ(1, commandLine.GetNumSwitchValues("remoteip"));
        EXPECT_STREQ("10.0.0.1", commandLine.GetSwitchValue("remoteip", 0).c_str());

        ASSERT_TRUE(commandLine.HasSwitch("scanfolders"));
        ASSERT_EQ(2, commandLine.GetNumSwitchValues("scanfolders"));
        EXPECT_STREQ(R"(\a\b\c)", commandLine.GetSwitchValue("scanfolders", 0).c_str());
        EXPECT_STREQ(R"(\d\e\f)", commandLine.GetSwitchValue("scanfolders", 1).c_str());

        ASSERT_EQ(3, commandLine.GetNumMiscValues());
        EXPECT_STREQ("programname.exe", commandLine.GetMiscValue(0).c_str());
        EXPECT_STREQ("Foo", commandLine.GetMiscValue(1).c_str());
        EXPECT_STREQ("Bat", commandLine.GetMiscValue(2).c_str());
    }

    TEST_F(SettingsRegistryMergeUtilsCommandLineFixture, RegsetFileArgument_DoesNotMergeNUL)
    {
        AZStd::string regsetFile = AZ::IO::SystemFile::GetNullFilename();
        AZ::CommandLine commandLine;
        commandLine.Parse({ "--regset-file", regsetFile });

        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(*m_registry, commandLine, false);

        // Add a settings path to anchor loaded settings underneath
        regsetFile = AZStd::string::format("%s::/AnchorPath/Of/Settings", AZ::IO::SystemFile::GetNullFilename());
        commandLine.Parse({ "--regset-file", regsetFile });

        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(*m_registry, commandLine, false);
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::NoType, m_registry->GetType("/AnchorPath/Of/Settings"));
    }

    using SettingsRegistryAncestorDescendantOrEqualPathFixture = SettingsRegistryMergeUtilsCommandLineFixture;

    TEST_F(SettingsRegistryAncestorDescendantOrEqualPathFixture, ValidateThatAncestorOrDescendantOrPathWithTheSameValue_Succeeds)
    {
        EXPECT_TRUE(AZ::SettingsRegistryMergeUtils::IsPathAncestorDescendantOrEqual("/Amazon/AzCore/Bootstrap", "/Amazon/AzCore"));
        EXPECT_TRUE(AZ::SettingsRegistryMergeUtils::IsPathAncestorDescendantOrEqual("/Amazon/AzCore/Bootstrap", "/Amazon/AzCore/Bootstrap"));
        EXPECT_TRUE(AZ::SettingsRegistryMergeUtils::IsPathAncestorDescendantOrEqual("/Amazon/AzCore/Bootstrap", "/Amazon/AzCore/Bootstrap/project_path"));
        EXPECT_FALSE(AZ::SettingsRegistryMergeUtils::IsPathAncestorDescendantOrEqual("/Amazon/AzCore/Bootstrap", "/Amazon/Project/Settings/project_name"));
    }
}
