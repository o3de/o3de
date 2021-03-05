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
#include "Maestro_precompiled.h"
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
    AZ_TEST_CLASS_ALLOCATOR(CryMovieTestEnvironment);

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
