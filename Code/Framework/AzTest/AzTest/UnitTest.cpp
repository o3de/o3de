/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/UnitTest.h>

#if AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL
#include <csignal>
#endif // AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL

#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/OSAllocator.h>

namespace UnitTest
{
    void TraceBusHook::SetupEnvironment()
    {
#if AZ_TRAIT_UNITTEST_USE_TEST_RUNNER_ENVIRONMENT
        AZ::EnvironmentInstance inst = AZ::Test::GetPlatform().GetTestRunnerEnvironment();
        AZ::Environment::Attach(inst);
        m_createdAllocator = false;
#else
        if (!AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
        {
            AZ::AllocatorInstance<AZ::OSAllocator>::Create(); // used by the bus
            m_createdAllocator = true;
        }
#endif
        BusConnect();

        m_environmentSetup = true;
    }

    void TraceBusHook::TeardownEnvironment()
    {
        if (m_environmentSetup)
        {
            BusDisconnect();

            if (m_createdAllocator)
            {
                AZ::AllocatorInstance<AZ::OSAllocator>::Destroy(); // used by the bus
            }

            // At this point, the AllocatorManager should not have any allocators left. If we happen to have any,
            // we exit the test with an error code (this way the test process does not return 0 and the test run
            // is considered a failure).
            AZ::AllocatorManager& allocatorManager = AZ::AllocatorManager::Instance();
            const int numAllocators = allocatorManager.GetNumAllocators();
            int invalidAllocatorCount = 0;

            for (int i = 0; i < numAllocators; ++i)
            {
                if (!allocatorManager.GetAllocator(i)->IsLazilyCreated())
                {
                    invalidAllocatorCount++;
                }
            }

            if (invalidAllocatorCount && m_createdAllocator)
            {
                // Print the name of the allocators still in the AllocatorManager
                ColoredPrintf(COLOR_RED, "[     FAIL ] There are still %d registered non-lazy allocators:\n", invalidAllocatorCount);
                for (int i = 0; i < numAllocators; ++i)
                {
                    if (!allocatorManager.GetAllocator(i)->IsLazilyCreated())
                    {
                        ColoredPrintf(COLOR_RED, "\t\t%s\n", allocatorManager.GetAllocator(i)->GetName());
                    }
                }

                AZ::AllocatorManager::Destroy();
                m_environmentSetup = false;

#if AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL
                std::raise(SIGTERM);
#endif // AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL
            }

            AZ::AllocatorManager::Destroy();
            m_environmentSetup = false;
        }
    }

} // namespace UnitTest
