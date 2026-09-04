/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzTest/AzTest.h>
#include <Mocks/ITimerMock.h>
#include <Mocks/ICryPakMock.h>
#include <Mocks/IConsoleMock.h>
#include <AzCore/Memory/OSAllocator.h>

using ::testing::NiceMock;

class CryMovieTestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    virtual ~CryMovieTestEnvironment()
    {}

protected:
    void SetupEnvironment() override
    {
        m_stubEnv.pTimer = &m_stubTimer;
        m_stubEnv.pCryPak = &m_stubPak;
        m_stubEnv.pConsole = &m_stubConsole;
        gEnv = &m_stubEnv;
    }

    void TeardownEnvironment() override
    {}

private:
    SSystemGlobalEnvironment m_stubEnv;
    NiceMock<TimerMock> m_stubTimer;
    NiceMock<CryPakMock> m_stubPak;
    NiceMock<ConsoleMock> m_stubConsole;
};

AZ_UNIT_TEST_HOOK(new CryMovieTestEnvironment)

TEST(CryMovieSanityTest, Sanity)
{
    EXPECT_EQ(1, 1);
}
