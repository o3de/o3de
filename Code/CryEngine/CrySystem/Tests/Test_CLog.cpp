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
#include <Log.h>

#include <Mocks/ISystemMock.h>
#include <Mocks/IRemoteConsoleMock.h>

#include <AzCore/IO/SystemFile.h> // for max path decl
#include <AzCore/Math/Random.h>
#include <AzCore/Memory/AllocatorScope.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>

namespace CLogUnitTests
{
    using ::testing::NiceMock;
    using ::testing::_;
    using ::testing::Return;
    
    // for fuzzing test, how much work to do?  Not much, as this must be fast.
    const int NumTrialsToPerform = 16000;

    class CLogUnitTests
        : public ::testing::Test
    {
    public:

        using CryPrimitivesAllocatorScope = AZ::AllocatorScope<AZ::LegacyAllocator, CryStringAllocator>;

        void SetUp() override
        {
            m_primitiveAllocators.ActivateAllocators();
            
            m_priorEnv = gEnv;
            m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            m_priorDirectFileIO = AZ::IO::FileIOBase::GetDirectInstance();

            m_data = AZStd::make_unique<DataMembers>();
            m_data->m_stubEnv.pSystem = &m_data->m_system;

            gEnv = &m_data->m_stubEnv;

            // for FileIO, you must set the instance to null before changing it.
            // this is a way to tell the singleton system that you mean to replace a singleton and its
            // not a mistake.
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(&m_data->m_fileIOMock);
            AZ::IO::FileIOBase::SetDirectInstance(nullptr);
            AZ::IO::FileIOBase::SetDirectInstance(&m_data->m_fileIOMock);

            ON_CALL(m_data->m_system, GetIRemoteConsole())
                .WillByDefault(
                    Return(&m_data->m_remoteConsoleMock));

            AZ::IO::MockFileIOBase::InstallDefaultReturns(m_data->m_fileIOMock);
        }

        void TearDown() override
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
            AZ::IO::FileIOBase::SetDirectInstance(nullptr);
            AZ::IO::FileIOBase::SetDirectInstance(m_priorDirectFileIO);

            m_data.reset();

            // restore state.
            gEnv = m_priorEnv;
            m_primitiveAllocators.DeactivateAllocators();
        }

        struct DataMembers
        {
            SSystemGlobalEnvironment m_stubEnv;
            NiceMock<SystemMock> m_system;
            NiceMock<AZ::IO::MockFileIOBase> m_fileIOMock;
            NiceMock<IRemoteConsoleMock> m_remoteConsoleMock;
        };

        AZStd::unique_ptr<DataMembers> m_data;
        SSystemGlobalEnvironment* m_priorEnv = nullptr;
        ISystem* m_priorSystem = nullptr;
        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZ::IO::FileIOBase* m_priorDirectFileIO = nullptr;
        CryPrimitivesAllocatorScope m_primitiveAllocators;
    };

    TEST_F(CLogUnitTests, LogAlways_InvalidString_Asserts)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        CLog testLog(&m_data->m_system);
        testLog.LogAlways(nullptr);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(CLogUnitTests, LogAlways_EmptyString_IgnoresWithoutCrashing)
    {
        CLog testLog(&m_data->m_system);
        testLog.LogAlways("");
    }

    TEST_F(CLogUnitTests, LogAlways_NormalString_NoFileName_DoesNotCrash)
    {
        CLog testLog(&m_data->m_system);
        testLog.LogAlways("test");
    }

    TEST_F(CLogUnitTests, LogAlways_SetFileName_Empty_DoesNotCrash)
    {
        CLog testLog(&m_data->m_system);
        testLog.SetFileName("", false);
        testLog.LogAlways("test");
    }

#if AZ_TRAIT_DISABLE_LOG_ALWAYS_FUZZ_TEST
    TEST_F(CLogUnitTests, DISABLED_LogAlways_FuzzTest)
#else
    TEST_F(CLogUnitTests, LogAlways_FuzzTest)
#endif // AZ_TRAIT_DISABLE_LOG_ALWAYS_FUZZ_TEST
    {
        CLog testLog(&m_data->m_system);
        AZStd::string randomJunkName;

        randomJunkName.resize(128, '\0');

        // expect the mock to repeatedly get called.  If we fail this expectation
        // it means the code is early-outing somewhere and we are not getting coverage.
        EXPECT_CALL(m_data->m_fileIOMock, Write(_, _, _, _))
            .WillRepeatedly(
                Return(AZ::IO::Result(AZ::IO::ResultCode::Success)));

        // don't rely on randomness in unit tests, they need to be repeatable.
        // the following random generator is not seeded by the time, but by a constant (default 1234).
        AZ::SimpleLcgRandom randGen;

        for (int trialNumber = 0; trialNumber < NumTrialsToPerform; ++trialNumber)
        {
            for (int randomChar = 0; randomChar < randomJunkName.size(); ++randomChar)
            {
                // note that this is intentionally allowing null characters to generate.
                // note that this also puts characters AFTER the null, if a null appears in the mddle.
                // so that if there are off by one errors they could include cruft afterwards.

                if (randomChar > trialNumber % randomJunkName.size())
                {
                    // choose this point for the nulls to begin.  It makes sure we test every size of string.
                    randomJunkName[randomChar] = 0;
                }
                else
                {
                    randomJunkName[randomChar] = (char)(randGen.GetRandom() % 256); // this will trigger invalid UTF8 decoding too
                }
            }
            testLog.LogAlways("%s", randomJunkName.c_str());
        }
    }

    TEST_F(CLogUnitTests, LogAlways_SetFileName_Correct_DoesNotCrash_WritesToFile)
    {
        CLog testLog(&m_data->m_system);
        testLog.SetFileName("logfile.log", false);

        // EXPECT a call to the file system - if we dont get a call here, it means something went wrong.
        // it also expects exactly one call to write.  One call to log should be one call to write,
        // or else performance will suffer.

        EXPECT_CALL(m_data->m_fileIOMock, Write(_, _, _, _))
            .WillOnce(
                Return(AZ::IO::Result(AZ::IO::ResultCode::Success)));

        testLog.LogAlways("test");
    }
} // end namespace CLogUnitTests


