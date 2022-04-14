/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AutomationApplicationFixture.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/UnitTest/UnitTest.h>


namespace UnitTest
{
    TEST_F(AutomationApplicationFixture, AutomationInterface_CustomBehaviorContext_HasCoreMethods)
    {
        CreateApplication({ });

        auto automationSystem = Automation::AutomationInterface::Get();
        ASSERT_TRUE(automationSystem);

        auto behaviorContext = automationSystem->GetAutomationContext();
        ASSERT_TRUE(behaviorContext);

        EXPECT_TRUE(behaviorContext->m_methods.find("Print") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("Warning") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("Error") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("ExecuteConsoleCommand") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("IdleFrames") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("IdleSeconds") != behaviorContext->m_methods.end());
    }
} // namespace UnitTest

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
