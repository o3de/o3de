/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef GM_UNITTEST_FIXTURE_H
#define GM_UNITTEST_FIXTURE_H

#include <GridMate/GridMate.h>
#include <GridMate/Session/Session.h>

#include <GridMateTests/Tests_Platform.h>
#include <AzCore/UnitTest/UnitTest.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/AllocationRecords.h>

#include <GridMate/Carrier/Carrier.h>

#include <AzCore/AzCore_Traits_Platform.h>

//////////////////////////////////////////////////////////////////////////

namespace UnitTest
{
    struct TestCarrierDesc
        : GridMate::CarrierDesc
    {
        TestCarrierDesc() : CarrierDesc()
        {
            m_connectionTimeoutMS = 15000;
        }
    };

    class GridMateTestFixture 
        : public GridMateTestFixture_Platform
    {
        protected:
            GridMate::IGridMate* m_gridMate;
    
        private:
            using Platform = GridMateTestFixture_Platform;

    public:
        GridMateTestFixture([[maybe_unused]] unsigned int memorySize = 100 * 1024 * 1024)
            : m_gridMate(nullptr)
        {
            GridMate::GridMateDesc desc;
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

            //desc.m_autoInitPlatformNetModules = false;
            m_gridMate = GridMateCreate(desc);
            AZ_TEST_ASSERT(m_gridMate != NULL);

            AZ::AllocatorManager::Instance().EnterProfilingMode();
            AZ::Debug::AllocationRecords* records = AZ::AllocatorInstance<GridMate::GridMateAllocator>::GetAllocator().GetRecords();
            if (records)
            {
                records->SetMode(AZ::Debug::AllocationRecords::RECORD_FULL);
            }

            Platform::Construct();
        }

        virtual ~GridMateTestFixture()
        {
            Platform::Destruct();

            if (m_gridMate)
            {
                GridMateDestroy(m_gridMate);
                m_gridMate = NULL;
            }

            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            AZ::AllocatorManager::Instance().ExitProfilingMode();
        }

        void Update()
        {
        }
    };

    /**
    * Since we test many MP modules that require MP memory, but
    * we don't want to start a real MP service. We just "hack" the
    * system and initialize the GridMate MP allocator manually.
    * If you try to use m_gridMate->StartMultiplayerService the code will Warn that the allocator
    * has been created!
    */
    class GridMateMPTestFixture
        : public GridMateTestFixture
    {
    public:
        GridMateMPTestFixture(unsigned int memorySize = 100* 1024* 1024, bool needWSA = true)
            : GridMateTestFixture(memorySize)
            , m_needWSA(needWSA)
        {
            if (m_gridMate)
            {
#if AZ_TRAIT_OS_USE_WINDOWS_SOCKETS
                if (m_needWSA)
                {
                    WSAData wsaData;
                    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
                    if (err != 0)
                    {
                        AZ_TracePrintf("GridMate", "GridMateMPTestFixture: Failed on WSAStartup with code %d\n", err);
                    }
                }
#endif
                AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create();

                AZ::Debug::AllocationRecords* records = AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::GetAllocator().GetRecords();
                if (records)
                {
                    records->SetMode(AZ::Debug::AllocationRecords::RECORD_FULL);
                }
            }
        }

        ~GridMateMPTestFixture()
        {
            if (m_gridMate)
            {
                AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();
            }

#if AZ_TRAIT_OS_USE_WINDOWS_SOCKETS
            if (m_needWSA)
            {
                WSACleanup();
            }
#endif
        }

        bool m_needWSA;
    };
}

// Wraps AZTest to a more GoogleTest format
#define GM_TEST_SUITE(...) namespace __VA_ARGS__ { namespace UnitTest {
#define GM_TEST(...) TEST(__VA_ARGS__, __VA_ARGS__) { ::UnitTest::__VA_ARGS__ tester; tester.run(); }
#define GM_TEST_SUITE_END(...) } }

#endif // GM_UNITTEST_FIXTURE_H
