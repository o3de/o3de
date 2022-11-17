/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzTest/AzTest.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Application/Application.h>

#include <AudioSystemImpl_wwise.h>
#include <AudioEngineWwise_Traits_Platform.h>
#include <Config_wwise.h>

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkMemoryMgr.h>
#include <AK/SoundEngine/Common/AkModule.h>

#include <platform.h>
#include <Mocks/IConsoleMock.h>
#include <Mocks/ISystemMock.h>

using namespace Audio;
using ::testing::NiceMock;

namespace UnitTest
{
    class WwiseTestEnvironment
        : public AZ::Test::ITestEnvironment
    {
    public:
        AZ_TEST_CLASS_ALLOCATOR(WwiseTestEnvironment)

        WwiseTestEnvironment() = default;
        ~WwiseTestEnvironment() override = default;

    protected:

        struct MockHolder
        {
            AZ_TEST_CLASS_ALLOCATOR(MockHolder)

            NiceMock<ConsoleMock> m_console;
            NiceMock<SystemMock> m_system;
        };

        void SetupEnvironment() override
        {
            // Setup Mocks on a stub environment
            m_mocks = new MockHolder();

            m_stubEnv.pConsole = &m_mocks->m_console;
            m_stubEnv.pSystem = &m_mocks->m_system;
            gEnv = &m_stubEnv;
        }

        void TeardownEnvironment() override
        {
            delete m_mocks;
            m_mocks = nullptr;
        }

    private:
        SSystemGlobalEnvironment m_stubEnv;
        MockHolder* m_mocks = nullptr;
    };

    class AudioSystemImplWwiseTests
        : public ::testing::Test
    {
    public:
        AZ_TEST_CLASS_ALLOCATOR(AudioSystemImplWwiseTests)

    protected:
        AudioSystemImplWwiseTests()
            : m_wwiseImpl("")
        {}

        void SetUp() override
        {
#if !AZ_TRAIT_DISABLE_FAILED_AUDIO_WWISE_TESTS
            // Init Wwise
            AkMemSettings memSettings;
            AK::MemoryMgr::GetDefaultSettings(memSettings);
            AK::MemoryMgr::Init(&memSettings);
            AkStreamMgrSettings strmSettings;
            AK::StreamMgr::GetDefaultSettings(strmSettings);
            AK::StreamMgr::Create(strmSettings);
            AkInitSettings initSettings;
            AK::SoundEngine::GetDefaultInitSettings(initSettings);
            AkPlatformInitSettings platSettings;
            AK::SoundEngine::GetDefaultPlatformInitSettings(platSettings);
            AK::SoundEngine::Init(&initSettings, &platSettings);
#endif // !AZ_TRAIT_DISABLE_FAILED_AUDIO_WWISE_TESTS
        }

        void TearDown() override
        {
#if !AZ_TRAIT_DISABLE_FAILED_AUDIO_WWISE_TESTS
            // Term Wwise
            AK::SoundEngine::Term();
            AK::IAkStreamMgr::Get()->Destroy();
            AK::MemoryMgr::Term();
#endif // !AZ_TRAIT_DISABLE_FAILED_AUDIO_WWISE_TESTS
        }

        CAudioSystemImpl_wwise m_wwiseImpl;
    };

#if AZ_TRAIT_DISABLE_FAILED_AUDIO_WWISE_TESTS
    TEST_F(AudioSystemImplWwiseTests, DISABLED_WwiseSanityTest)
#else
    TEST_F(AudioSystemImplWwiseTests, WwiseSanityTest)
#endif // AZ_TRAIT_DISABLE_FAILED_AUDIO_WWISE_TESTS
    {
        // Tests that Setup/TearDown work as expected
    }

#if AZ_TRAIT_DISABLE_FAILED_AUDIO_WWISE_TESTS
    TEST_F(AudioSystemImplWwiseTests, DISABLED_WwiseMultiPosition_GoodData)
#else
    TEST_F(AudioSystemImplWwiseTests, WwiseMultiPosition_GoodData)
#endif // AZ_TRAIT_DISABLE_FAILED_AUDIO_WWISE_TESTS
    {
        SATLAudioObjectData_wwise wwiseObject(1, true);
        MultiPositionParams params;
        params.m_positions.push_back(AZ::Vector3(1.f, 2.f, 3.f));
        params.m_type = MultiPositionBehaviorType::Blended;

        auto result = m_wwiseImpl.SetMultiplePositions(&wwiseObject, params);
        EXPECT_EQ(result, EAudioRequestStatus::Success);
    }

#if AZ_TRAIT_AUDIOENGINEWWISE_DISABLE_MULTIPOSITION_TESTS
    TEST_F(AudioSystemImplWwiseTests, DISABLED_WwiseMultiPosition_BadObject)
#else
    TEST_F(AudioSystemImplWwiseTests, WwiseMultiPosition_BadObject)
#endif
    {
        MultiPositionParams params;
        params.m_positions.push_back(AZ::Vector3(1.f, 2.f, 3.f));
        params.m_type = MultiPositionBehaviorType::Separate;

        auto result = m_wwiseImpl.SetMultiplePositions(nullptr, params);
        EXPECT_EQ(result, EAudioRequestStatus::Failure);
    }

#if AZ_TRAIT_DISABLE_FAILED_AUDIO_WWISE_TESTS
    TEST_F(AudioSystemImplWwiseTests, DISABLED_WwiseMultiPosition_ZeroPositions)
#else
    TEST_F(AudioSystemImplWwiseTests, WwiseMultiPosition_ZeroPositions)
#endif // AZ_TRAIT_DISABLE_FAILED_AUDIO_WWISE_TESTS
    {
        SATLAudioObjectData_wwise wwiseObject(1, true);
        MultiPositionParams params;
        auto result = m_wwiseImpl.SetMultiplePositions(&wwiseObject, params);
        EXPECT_EQ(result, EAudioRequestStatus::Success);
    }


    class AudioSystemImpl_wwise_Test
        : public CAudioSystemImpl_wwise
    {
    public:
        AZ_TEST_CLASS_ALLOCATOR(AudioSystemImpl_wwise_Test)

        explicit AudioSystemImpl_wwise_Test(const char* assetPlatform)
            : CAudioSystemImpl_wwise(assetPlatform)
        {}

#if AZ_TRAIT_DISABLE_FAILED_AUDIO_WWISE_TESTS
        friend class GTEST_TEST_CLASS_NAME_(AudioSystemImplWwiseConfigTests, DISABLED_WwiseSetBankPaths_NonDefaultPath_PathMatches);
        friend class GTEST_TEST_CLASS_NAME_(AudioSystemImplWwiseConfigTests, DISABLED_WwiseSetBankPaths_NoInitBnk_DefaultPath);
#else
        friend class GTEST_TEST_CLASS_NAME_(AudioSystemImplWwiseConfigTests, WwiseSetBankPaths_NonDefaultPath_PathMatches);
        friend class GTEST_TEST_CLASS_NAME_(AudioSystemImplWwiseConfigTests, WwiseSetBankPaths_NoInitBnk_DefaultPath);
#endif // AZ_TRAIT_DISABLE_FAILED_AUDIO_WWISE_TESTS
    };


    class AudioSystemImplWwiseConfigTests
        : public ::testing::Test
    {
    public:
        AZ_TEST_CLASS_ALLOCATOR(AudioSystemImplWwiseConfigTests)

    protected:
        AudioSystemImplWwiseConfigTests()
            : m_wwiseImpl("")
        {
        }

        void SetUp() override
        {
            m_app.Start(AZ::ComponentApplication::Descriptor());

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            // Store and remove the existing fileIO...
            m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
            if (m_prevFileIO)
            {
                AZ::IO::FileIOBase::SetInstance(nullptr);
            }

            // Replace with a new LocalFileIO...
            m_fileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
            AZ::IO::FileIOBase::SetInstance(m_fileIO.get());

            // Reflect the wwise config settings...
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_TEST_ASSERT(serializeContext != nullptr);
            Audio::Wwise::ConfigurationSettings::Reflect(serializeContext);

            // Set the @products@ alias to the path where *this* cpp file lives.
            AZStd::string rootFolder(AZ::Test::GetCurrentExecutablePath());
            AZ::StringFunc::Path::Join(rootFolder.c_str(), "Test.Assets/Gems/AudioEngineWwise", rootFolder);
            m_fileIO->SetAlias("@products@", rootFolder.c_str());

            // So we don't have to compute it in each test...
            AZStd::string defaultBanksPath(Audio::Wwise::DefaultBanksPath);
            m_configFilePath = AZStd::string::format("%s/%s%s", rootFolder.c_str(), defaultBanksPath.c_str(), Audio::Wwise::ConfigFile);
        }

        void TearDown() override
        {
            // Destroy our LocalFileIO...
            m_fileIO.reset();

            // Replace the old fileIO (set instance to null first)...
            AZ::IO::FileIOBase::SetInstance(nullptr);
            if (m_prevFileIO)
            {
                AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
                m_prevFileIO = nullptr;
            }

            m_app.Stop();
        }

        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIO;
        AZStd::string m_configFilePath;
        Audio::Wwise::ConfigurationSettings::PlatformMapping m_mapEntry;
        AudioSystemImpl_wwise_Test m_wwiseImpl;

    private:
        AzFramework::Application m_app;
        AZ::IO::FileIOBase* m_prevFileIO { nullptr };
    };


#if AZ_TRAIT_DISABLE_FAILED_AUDIO_WWISE_TESTS
    TEST_F(AudioSystemImplWwiseConfigTests, DISABLED_WwiseSetBankPaths_NonDefaultPath_PathMatches)
#else
    TEST_F(AudioSystemImplWwiseConfigTests, WwiseSetBankPaths_NonDefaultPath_PathMatches)
#endif // AZ_TRAIT_DISABLE_FAILED_AUDIO_WWISE_TESTS
    {
        // The mapping here points to a custom directory that exists (and contains an init.bnk).
        // The custom bank path should be set.
        Audio::Wwise::ConfigurationSettings config;
        m_mapEntry.m_enginePlatform = AZ_TRAIT_OS_PLATFORM_NAME;
        m_mapEntry.m_bankSubPath = "soundbanks";
        config.m_platformMappings.push_back(m_mapEntry);

        config.Save(m_configFilePath);

        m_wwiseImpl.SetBankPaths();

        m_fileIO->Remove(m_configFilePath.c_str());

        EXPECT_STREQ(m_wwiseImpl.m_soundbankFolder.c_str(), "sounds/wwise/soundbanks/");
    }


#if AZ_TRAIT_DISABLE_FAILED_AUDIO_WWISE_TESTS
    TEST_F(AudioSystemImplWwiseConfigTests, DISABLED_WwiseSetBankPaths_NoInitBnk_DefaultPath)
#else
    TEST_F(AudioSystemImplWwiseConfigTests, WwiseSetBankPaths_NoInitBnk_DefaultPath)
#endif // AZ_TRAIT_DISABLE_FAILED_AUDIO_WWISE_TESTS
    {
        // The mapping here points to a directory that does not exist (and doesn't contain init.bnk).
        // The default bank path should be set.
        Audio::Wwise::ConfigurationSettings config;
        m_mapEntry.m_enginePlatform = AZ_TRAIT_OS_PLATFORM_NAME;
        m_mapEntry.m_bankSubPath = "no_soundbanks";
        config.m_platformMappings.push_back(m_mapEntry);
        config.Save(m_configFilePath);

        m_wwiseImpl.SetBankPaths();

        m_fileIO->Remove(m_configFilePath.c_str());
        EXPECT_STREQ(m_wwiseImpl.m_soundbankFolder.c_str(), Audio::Wwise::DefaultBanksPath);
    }

} // namespace UnitTest


AZ_UNIT_TEST_HOOK(new UnitTest::WwiseTestEnvironment);
