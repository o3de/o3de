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
#include "RenderDll_precompiled.h"
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Memory/AllocatorScope.h>
#include <AzCore/UnitTest/UnitTest.h>
#include "Mocks/IConsoleMock.h"
#include "Mocks/ICVarMock.h"
#include "Mocks/ISystemMock.h"
#include "RemoteCompiler.h"

namespace NRemoteCompiler
{
    using ::testing::NiceMock;
    using ::testing::Return;

    using SystemAllocatorScope = AZ::AllocatorScope<AZ::LegacyAllocator, CryStringAllocator>;

    // define a dummy compressor decompressor that simply does nothing.
    // the actual ISystem::Decompress / Compress should be tested in isolation in the unit it lives in.
    // this is expected to fit both of these functions:
    //virtual bool CompressDataBlock(const void* input, size_t inputSize, void* output, size_t& outputSize, int level = 3) = 0;
    //virtual bool DecompressDataBlock(const void* input, size_t inputSize, void* output, size_t& outputSize) = 0;
    // it operates by copying the input into the output and the input size into the output size and always returning true.
    AZ_PUSH_DISABLE_WARNING(4100, "-Wunknown-warning-option")
    ACTION(MockCompressDecompress)
    {
        AZ_Assert(arg3 >= arg1, "MockCompressDecompress would overrun buffer (%i must be >= %i)", (int)arg3, (int)arg1);
        if (arg3 < arg1)
        {
            return false;
        }

        memcpy(arg2, arg0, arg1);
        arg3 = arg1;
        return true;
    };
    AZ_POP_DISABLE_WARNING

    using ::UnitTest::AllocatorsTestFixture;

    class RemoteCompilerTest
        : public AllocatorsTestFixture
        , public SystemAllocatorScope 
    {
    public:

        const int m_fakePortNumber = 12345;

        void SetUp() override
        {
            using namespace ::testing;

            AllocatorsTestFixture::SetUp();
            SystemAllocatorScope::ActivateAllocators();

            m_priorEnv = gEnv;

            m_data.reset(new DataMembers);

            ON_CALL(m_data->m_console, GetCVar(_))
                .WillByDefault(Return(&m_data->m_cvarMock));

            ON_CALL(m_data->m_system, CompressDataBlock(_, _, _, _, _))
                .WillByDefault(MockCompressDecompress());

            ON_CALL(m_data->m_system, DecompressDataBlock(_, _, _, _))
                .WillByDefault(MockCompressDecompress());

            ON_CALL(m_data->m_cvarMock, GetIVal())
                .WillByDefault(Return(m_fakePortNumber));

            memset(&m_data->m_stubEnv, 0, sizeof(SSystemGlobalEnvironment));
            m_data->m_stubEnv.pConsole = &m_data->m_console;
            m_data->m_stubEnv.pSystem = &m_data->m_system;
            gEnv = &m_data->m_stubEnv;
        }

        void TearDown() override
        {
            gEnv = m_priorEnv;
            m_data.reset();
            SystemAllocatorScope::DeactivateAllocators();
            AllocatorsTestFixture::TearDown();
        }

        struct DataMembers
        {
            NiceMock<SystemMock> m_system;
            NiceMock<ConsoleMock> m_console;
            NiceMock<CVarMock> m_cvarMock;
            SSystemGlobalEnvironment m_stubEnv;
        };

        AZStd::unique_ptr<DataMembers> m_data;

        SSystemGlobalEnvironment* m_priorEnv = nullptr;
    };

    // allow punch through to PRIVATE functions so that they do not need to be made PUBLIC.
    class ShaderSrvUnitTestAccessor
        : public CShaderSrv
    {
    public:
        ShaderSrvUnitTestAccessor()
            : CShaderSrv()
        {
            CShaderSrv::EnableUnitTestingMode(true);
        }
        EServerError SendRequestViaEngineConnection(std::vector<uint8>& rCompileData)   const
        {
            return CShaderSrv::SendRequestViaEngineConnection(rCompileData);
        }

        bool EncapsulateRequestInEngineConnectionProtocol(std::vector<uint8>& rCompileData) const
        {
            return CShaderSrv::EncapsulateRequestInEngineConnectionProtocol(rCompileData);
        }
    };

    TEST_F(RemoteCompilerTest, CShaderSrv_Constructor_WithNoGameName_Fails)
    {
        EXPECT_CALL(m_data->m_cvarMock, GetString());

        AZ_TEST_START_TRACE_SUPPRESSION;
        ShaderSrvUnitTestAccessor srv;
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(RemoteCompilerTest, CShaderSrv_Constructor_WithValidGameName_Succeeds)
    {
        // when we construct the server it calls get on the game name
        EXPECT_CALL(m_data->m_cvarMock, GetString())
            .WillOnce(Return("StarterGame"));

        ShaderSrvUnitTestAccessor srv;
    }

    TEST_F(RemoteCompilerTest, CShaderSrv_EncapsulateRequestInEngineConnectionProtocol_EmptyData_Fails)
    {
        // when we construct the server it calls get on the game name
        EXPECT_CALL(m_data->m_cvarMock, GetString())
            .WillOnce(Return("StarterGame"));

        ShaderSrvUnitTestAccessor srv;
        

        std::vector<uint8> testVector;

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(srv.EncapsulateRequestInEngineConnectionProtocol(testVector)); // empty vector error condition
        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // expect the above to have thrown an AZ_ERROR
    }

    TEST_F(RemoteCompilerTest, CShaderSrv_EncapsulateRequestInEngineConnectionProtocol_ValidData_EmptyServerList_Fails)
    {
        // when we construct the server it calls get on the game name
        EXPECT_CALL(m_data->m_cvarMock, GetString())
            .WillOnce(Return("StarterGame"));

        ShaderSrvUnitTestAccessor srv;
        

        EXPECT_CALL(m_data->m_cvarMock, GetString())
            .WillRepeatedly(Return("")); // empty server list

        std::vector<uint8> testVector;
        std::string testString("_-=!-");
        testVector.insert(testVector.begin(), testString.begin(), testString.end());

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(srv.EncapsulateRequestInEngineConnectionProtocol(testVector)); // empty vector error condition
        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // expect the above to have thrown an AZ_ERROR
    }

    TEST_F(RemoteCompilerTest, CShaderSrv_EncapsulateRequestInEngineConnectionProtocol_ValidInputs_Succeeds)
    {
        // when we construct the server it calls get on the game name
        EXPECT_CALL(m_data->m_cvarMock, GetString())
            .WillOnce(Return("StarterGame"));

        ShaderSrvUnitTestAccessor srv;
        

        // After this, it will repeatedly call get cvar to get the server address:
        const char* testList = "10.20.30.40";
        EXPECT_CALL(m_data->m_cvarMock, GetString())
            .WillRepeatedly(Return(testList));

        std::vector<uint8> testVector;
        std::string testString("_-=!-");
        testVector.insert(testVector.begin(), testString.begin(), testString.end());

        EXPECT_TRUE(srv.EncapsulateRequestInEngineConnectionProtocol(testVector));
    }

    TEST_F(RemoteCompilerTest, CShaderSrv_SendRequestViaEngineConnection_EmptyData_Fails)
    {
        // when we construct the server it calls get on the game name
        EXPECT_CALL(m_data->m_cvarMock, GetString())
            .WillOnce(Return("StarterGame"));

        ShaderSrvUnitTestAccessor srv;
        

        // After this, it will repeatedly call get cvar to get the server address:
        const char* testList = "10.20.30.40";
        EXPECT_CALL(m_data->m_cvarMock, GetString())
            .WillRepeatedly(Return(testList));

        std::vector<uint8> testVector;
        std::string testString("empty");

        // test for empty data - recvfailed expected (error emitted)
        AZ_TEST_START_TRACE_SUPPRESSION;
        testString = "empty";
        testVector.assign(testString.begin(), testString.end());
        EXPECT_EQ(srv.SendRequestViaEngineConnection(testVector), EServerError::ESRecvFailed);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(RemoteCompilerTest, CShaderSrv_SendRequestViaEngineConnection_IncompleteData_Fails)
    {
        // when we construct the server it calls get on the game name
        EXPECT_CALL(m_data->m_cvarMock, GetString())
            .WillOnce(Return("StarterGame"));

        ShaderSrvUnitTestAccessor srv;
        

        // After this, it will repeatedly call get cvar to get the server address:
        const char* testList = "10.20.30.40";
        EXPECT_CALL(m_data->m_cvarMock, GetString())
            .WillRepeatedly(Return(testList));

        std::vector<uint8> testVector;
        std::string testString("incomplete");
        testVector.assign(testString.begin(), testString.end());

        // test for incomplete data - recvfailed expected
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_EQ(srv.SendRequestViaEngineConnection(testVector), EServerError::ESRecvFailed);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(RemoteCompilerTest, CShaderSrv_SendRequestViaEngineConnection_CorruptData_Fails)
    {
        // when we construct the server it calls get on the game name
        EXPECT_CALL(m_data->m_cvarMock, GetString())
            .WillOnce(Return("StarterGame"));

        ShaderSrvUnitTestAccessor srv;
        

        // After this, it will repeatedly call get cvar to get the server address:
        const char* testList = "10.20.30.40";
        EXPECT_CALL(m_data->m_cvarMock, GetString())
            .WillRepeatedly(Return(testList));

        std::vector<uint8> testVector;
        std::string testString("corrupt");
        testVector.assign(testString.begin(), testString.end());

        // test for incomplete data - recvfailed expected
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_EQ(srv.SendRequestViaEngineConnection(testVector), EServerError::ESRecvFailed);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(RemoteCompilerTest, CShaderSrv_SendRequestViaEngineConnection_CompileError_Fails_ReturnsText)
    {
        // when we construct the server it calls get on the game name
        EXPECT_CALL(m_data->m_cvarMock, GetString())
            .WillOnce(Return("StarterGame"));

        ShaderSrvUnitTestAccessor srv;
        

        // After this, it will repeatedly call get cvar to get the server address:
        const char* testList = "10.20.30.40";
        EXPECT_CALL(m_data->m_cvarMock, GetString())
            .WillRepeatedly(Return(testList));

        std::vector<uint8> testVector;
        std::string testString("corrupt");
        testVector.assign(testString.begin(), testString.end());
        // test for an actual compile error - decompressed compile erro rexpected to be attached.
        testString = "compile_failure";
        testVector.assign(testString.begin(), testString.end());
        EXPECT_EQ(srv.SendRequestViaEngineConnection(testVector), EServerError::ESCompileError);
        // validate hte compile erorr decompressed successfully
        const char* expected_decode = "decompressed_plaintext";
        EXPECT_EQ(testVector.size(), strlen(expected_decode));
        EXPECT_EQ(memcmp(testVector.data(), expected_decode, strlen(expected_decode)), 0);
    }

    TEST_F(RemoteCompilerTest, CShaderSrv_SendRequestViaEngineConnection_ValidInput_Succeeds_ReturnsText)
    {
        // when we construct the server it calls get on the game name
        EXPECT_CALL(m_data->m_cvarMock, GetString())
            .WillOnce(Return("StarterGame"));

        ShaderSrvUnitTestAccessor srv;
        

        // After this, it will repeatedly call get cvar to get the server address:
        const char* testList = "10.20.30.40";
        EXPECT_CALL(m_data->m_cvarMock, GetString())
            .WillRepeatedly(Return(testList));

        AZStd::string testString = "success";
        std::vector<uint8> testVector;
        testVector.assign(testString.begin(), testString.end());

        EXPECT_EQ(srv.SendRequestViaEngineConnection(testVector), EServerError::ESOK);

        // validate that the result decompressed successfully - its expected to contain "decompressed_plaintext"
        const char* expected_decode = "decompressed_plaintext";
        EXPECT_EQ(testVector.size(), strlen(expected_decode));
        EXPECT_EQ(memcmp(testVector.data(), expected_decode, strlen(expected_decode)), 0);
    }
}
