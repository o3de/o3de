/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "CrySystem_precompiled.h"
#include <AzTest/AzTest.h>
#include <AzCore/Memory/AllocatorScope.h>
#include "LocalizedStringManager.h"
#include <Mocks/ISystemMock.h>
#include <Mocks/IConsoleMock.h>
#include <Mocks/ICryPakMock.h>
#include <Mocks/ICVarMock.h>

#include <vector>

class SystemEventDispatcherMock
    : public ISystemEventDispatcher
{
public:
    virtual ~SystemEventDispatcherMock() {}
    MOCK_METHOD1(RegisterListener, bool(ISystemEventListener* pListener));
    MOCK_METHOD1(RemoveListener, bool(ISystemEventListener* pListener));
    MOCK_METHOD3(OnSystemEvent, void(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam));
    MOCK_METHOD0(Update, void());
};

using namespace testing;
using ::testing::NiceMock;

using SystemAllocatorScope = AZ::AllocatorScope<AZ::LegacyAllocator, CryStringAllocator>;

class SystemFixture
    : public ::testing::Test
    , public SystemAllocatorScope
{
public:
    SystemFixture()
    {
        EXPECT_CALL(m_system, GetISystemEventDispatcher())
            .WillRepeatedly(Return(&m_dispatcher));

        EXPECT_CALL(m_console, GetCVar(_))
            .WillRepeatedly(Return(&m_cvarMock));

        EXPECT_CALL(m_cryPak, FindFirst(_, _, _))
            .WillRepeatedly(Return(AZ::IO::ArchiveFileIterator{}));

        EXPECT_CALL(m_cryPak, GetLocalizationFolder())
            .WillRepeatedly(Return("french"));

        EXPECT_CALL(m_cvarMock, GetFlags())
            .WillRepeatedly(Return(VF_WASINCONFIG));
    }

    void SetUp() override 
    {
        SystemAllocatorScope::ActivateAllocators();

        memset(&m_stubEnv, 0, sizeof(SSystemGlobalEnvironment));
        m_stubEnv.pConsole = &m_console;
        m_stubEnv.pSystem = &m_system;
        m_stubEnv.pCryPak = &m_cryPak;
        m_stubEnv.pLog = nullptr;
        m_priorEnv = gEnv;
        gEnv = &m_stubEnv;
    }

    void TearDown() override
    {
        gEnv = m_priorEnv;

        SystemAllocatorScope::DeactivateAllocators();
    }

    NiceMock<SystemMock> m_system;
    NiceMock<SystemEventDispatcherMock> m_dispatcher;
    NiceMock<ConsoleMock> m_console;
    NiceMock<CryPakMock> m_cryPak;
    NiceMock<CVarMock> m_cvarMock;
    SSystemGlobalEnvironment m_stubEnv;
    SSystemGlobalEnvironment* m_priorEnv = nullptr;
};

class UnitTestCLocalizedStringsManager : public CLocalizedStringsManager
{
public:
    UnitTestCLocalizedStringsManager(ISystem* pSystem) : CLocalizedStringsManager(pSystem)
    {
    }

    bool LocalizeLabel(const char* sLabel, string& outLocalizedString, bool bEnglish = false) override
    {
        m_capturedLabels.push_back(sLabel);
        return CLocalizedStringsManager::LocalizeLabel(sLabel, outLocalizedString, bEnglish);
    }

    std::vector<string> m_capturedLabels;

    friend class GTEST_TEST_CLASS_NAME_(SystemFixture, LocalizeStringInternal_WhitespaceCharacters_CorrectlyTokenizes);
};

// this test makes sure that whitespace characters such as tab work (not just space) and are considered to be separators.
TEST_F(SystemFixture, LocalizeStringInternal_SpecificWhitespaceCharacters_CorrectlyTokenizes)
{
    UnitTestCLocalizedStringsManager manager(&m_system);
    manager.SetLanguage("french");
    
    string outString;
    manager.LocalizeString_s("@hello\t@world", outString, false);
    ASSERT_EQ(manager.m_capturedLabels.size(), 2);
    EXPECT_STREQ(manager.m_capturedLabels[0].c_str(), "@hello");
    EXPECT_STREQ(manager.m_capturedLabels[1].c_str(), "@world");
    manager.m_capturedLabels.clear();
    
    manager.LocalizeString_s("@hello\n@world", outString, false);
    ASSERT_EQ(manager.m_capturedLabels.size(), 2);
    EXPECT_STREQ(manager.m_capturedLabels[0].c_str(), "@hello");
    EXPECT_STREQ(manager.m_capturedLabels[1].c_str(), "@world");
    manager.m_capturedLabels.clear();
    
    manager.LocalizeString_s("@hello\r@world", outString, false);
    ASSERT_EQ(manager.m_capturedLabels.size(), 2);
    EXPECT_STREQ(manager.m_capturedLabels[0].c_str(), "@hello");
    EXPECT_STREQ(manager.m_capturedLabels[1].c_str(), "@world");
    manager.m_capturedLabels.clear();
    
    manager.LocalizeString_s("@hello @world", outString, false);
    ASSERT_EQ(manager.m_capturedLabels.size(), 2);
    EXPECT_STREQ(manager.m_capturedLabels[0].c_str(), "@hello");
    EXPECT_STREQ(manager.m_capturedLabels[1].c_str(), "@world");
    manager.m_capturedLabels.clear();
}


// this test makes sure that multiple whitespace characters in a row don't themselves count as tokens or change the output in undesirable ways.
TEST_F(SystemFixture, LocalizeStringInternal_ManyWhitespaceCharacters_CorrectlyTokenizes)
{
    UnitTestCLocalizedStringsManager manager(&m_system);
    manager.SetLanguage("french");

    string outString;
    const char* testString = "@hello\n\r\t    \t\r\n@world\n\r\t    ";
    manager.LocalizeString_ch(testString, outString, false);
    ASSERT_EQ(manager.m_capturedLabels.size(), 2);
    EXPECT_STREQ(manager.m_capturedLabels[0].c_str(), "@hello");
    EXPECT_STREQ(manager.m_capturedLabels[1].c_str(), "@world");

    // since there are no localizations available it should not have gobbled up whitespace or altered it.
    EXPECT_STREQ(outString, testString);
}
