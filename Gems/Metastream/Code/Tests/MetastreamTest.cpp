/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <platform.h>
#include "Mocks/IConsoleMock.h"
#include "Mocks/ICVarMock.h"
#include "Mocks/ISystemMock.h"

#include <Metastream_Traits_Platform.h>
#include "MetastreamGem.h"

using ::testing::NiceMock;
using ::testing::Return;

class MetastreamTestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    AZ_TEST_CLASS_ALLOCATOR(MetastreamTestEnvironment);

    virtual ~MetastreamTestEnvironment()
    {}

protected:

    void SetupEnvironment() override
    {
        AZ::AllocatorInstance<AZ::OSAllocator>::Create();
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
    }

    void TeardownEnvironment() override
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
    }
};

AZ_UNIT_TEST_HOOK(new MetastreamTestEnvironment)


class MetastreamTest
    : public ::testing::Test
{
public:
    virtual ~MetastreamTest()
    {

    }

    const char* m_serverOptionsString = "document_root=Gems/Metastream/Files;listening_ports=8082";

protected:
    void SetUp() override
    {
        using namespace ::testing;

        m_priorEnv = gEnv;
        m_data.reset(new DataMembers);

        ON_CALL(m_data->m_console, GetCVar(_))
            .WillByDefault(::testing::Return(&m_data->m_cvarMock));

        ON_CALL(m_data->m_console, RegisterString(_, _, _, _, _))
            .WillByDefault(::testing::Return(&m_data->m_cvarMock));

        ON_CALL(m_data->m_cvarMock, GetString())
            .WillByDefault(::testing::Return(m_serverOptionsString));

        memset(&m_data->m_stubEnv, 0, sizeof(SSystemGlobalEnvironment));
        m_data->m_stubEnv.pConsole = &m_data->m_console;
        m_data->m_stubEnv.pSystem = &m_data->m_system;
        gEnv = &m_data->m_stubEnv;
    }

    void TearDown() override
    {
        gEnv = m_priorEnv;
        m_data.reset();
    }

    struct DataMembers
    {
        ::testing::NiceMock<SystemMock> m_system;
        ::testing::NiceMock<ConsoleMock> m_console;
        ::testing::NiceMock<CVarMock> m_cvarMock;
        SSystemGlobalEnvironment m_stubEnv;
    };

    AZStd::unique_ptr<DataMembers> m_data;

    SSystemGlobalEnvironment* m_priorEnv = nullptr;

};

// allow punch through to get at protected methods so they do not need to be made PUBLIC
class MetastreamTestAccessor
    : public Metastream::MetastreamGem
{
public:
    bool IsServerEnabled() const
    {
        return MetastreamGem::IsServerEnabled();
    }

    std::string GetDatabasesJSON() const
    {
        return MetastreamGem::GetDatabasesJSON();
    }
};

TEST_F(MetastreamTest, ServerStartupShutdownTest_FT)
{
    MetastreamTestAccessor server;

    EXPECT_FALSE(server.IsServerEnabled());

#if AZ_TRAIT_METASTREAM_USE_CIVET
    // Metastream only supported on PC
    EXPECT_FALSE(server.StartHTTPServer()); // fail because m_serverOptionsCVAR not declared until GAME_POST_INIT event

    server.OnSystemEvent(ESYSTEM_EVENT_GAME_POST_INIT, 0, 0); // fake this event to set the server options cvar
    EXPECT_TRUE(server.StartHTTPServer());
    EXPECT_TRUE(server.IsServerEnabled());

    EXPECT_EQ(server.GetDatabasesJSON(), "{\"tables\":[]}");
    server.AddBoolToCache("testtable", "testkey", true);
    EXPECT_EQ(server.GetDatabasesJSON(), "{\"tables\":[\"testtable\"]}");
#else
    EXPECT_FALSE(server.StartHTTPServer());
#endif

    // This will clear the cache
    server.StopHTTPServer();
    EXPECT_EQ(server.GetDatabasesJSON(), "{\"tables\":[]}");
    EXPECT_FALSE(server.IsServerEnabled());
}
