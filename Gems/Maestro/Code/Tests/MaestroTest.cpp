/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <Mocks/ICryPakMock.h>
#include <Mocks/IConsoleMock.h>
#include <AzCore/Memory/OSAllocator.h>

#include <ISystem.h>

using ::testing::NiceMock;

class MaestroTestEnvironment
    : public AZ::Test::ITestEnvironment
    , public UnitTest::TraceBusRedirector
{
public:
    AZ_TEST_CLASS_ALLOCATOR(MaestroTestEnvironment);

    virtual ~MaestroTestEnvironment()
    {}

protected:

    struct MockHolder
    {
        AZ_TEST_CLASS_ALLOCATOR(MockHolder);

        NiceMock<CryPakMock> pak;
        NiceMock<ConsoleMock> console;
    };

    void SetupEnvironment() override
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

        // Mocks need to be destroyed before the allocators are destroyed, 
        // but if they are member variables, they get destroyed *after*
        // TeardownEnvironment when this Environment class is destroyed
        // by the GoogleTest framework.
        //
        // Mocks also do not have any public destroy or release method to
        // manage their lifetime, so this solution manages the lifetime
        // and ordering via the heap.
        m_mocks = new MockHolder();
        m_stubEnv.pCryPak = &m_mocks->pak;
        m_stubEnv.pConsole = &m_mocks->console;
        gEnv = &m_stubEnv;

        BusConnect();
    }

    void TeardownEnvironment() override
    {
        BusDisconnect();
        // Destroy mocks before AZ allocators
        delete m_mocks;

        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }

private:
    SSystemGlobalEnvironment m_stubEnv;
    MockHolder* m_mocks;
};

AZ_UNIT_TEST_HOOK(new MaestroTestEnvironment)
