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
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <LegacyAllocator.h>
#include <System.h>

namespace UnitTests
{
    class CSystemUnitTests
        : public ::testing::Test
    {
    public:
        void SetUp() override
        {
            SSystemInitParams startupParams;
            AZ::AllocatorInstance<AZ::LegacyAllocator>::Create();
            AZ::AllocatorInstance<CryStringAllocator>::Create();
            m_system = new CSystem(startupParams.pSharedEnvironment);
        }

        void TearDown() override
        {
            delete m_system;
            AZ::AllocatorInstance<CryStringAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::LegacyAllocator>::Destroy();
        }


        CSystem* m_system = nullptr;
    };

    TEST_F(CSystemUnitTests, ApplicationLogInstanceUnitTests)
    {
        const char dummyString[] = "dummy";
        const char testString[] = "test";
        EXPECT_EQ(m_system->GetApplicationLogInstance(dummyString), 0);
        EXPECT_EQ(m_system->GetApplicationLogInstance(testString), 0);
#if AZ_TRAIT_OS_USE_WINDOWS_MUTEX
        EXPECT_EQ(m_system->GetApplicationLogInstance(dummyString), 1);
#endif
    }
}
