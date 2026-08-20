/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/InputDeviceId.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace InputDeviceIdReflectionTests
{
    namespace
    {
        bool g_constructorArgumentsPreserved = false;
    }

    class InputDeviceIdReflectionTest
        : public UnitTest::LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();

            g_constructorArgumentsPreserved = false;
            m_behaviorContext = aznew AZ::BehaviorContext();
            AzFramework::InputDeviceId::Reflect(m_behaviorContext);
            m_behaviorContext->Property(
                "constructorArgumentsPreserved",
                BehaviorValueProperty(&g_constructorArgumentsPreserved));

            m_scriptContext = aznew AZ::ScriptContext();
            m_scriptContext->BindTo(m_behaviorContext);
        }

        void TearDown() override
        {
            delete m_scriptContext;
            m_scriptContext = nullptr;
            delete m_behaviorContext;
            m_behaviorContext = nullptr;

            UnitTest::LeakDetectionFixture::TearDown();
        }

        AZ::BehaviorContext* m_behaviorContext = nullptr;
        AZ::ScriptContext* m_scriptContext = nullptr;
    };

    TEST_F(InputDeviceIdReflectionTest, ConstructorWithNoArguments_CreatesDefaultIdInLua)
    {
        const bool executed = m_scriptContext->Execute(
            "local deviceId = InputDeviceId()\n"
            "constructorArgumentsPreserved = "
            "deviceId.name == '' and deviceId.index == 0\n");

        EXPECT_TRUE(executed);
        EXPECT_TRUE(g_constructorArgumentsPreserved);
    }

    TEST_F(InputDeviceIdReflectionTest, ConstructorWithName_PreservesNameAndUsesDefaultIndexInLua)
    {
        const bool executed = m_scriptContext->Execute(
            "local deviceId = InputDeviceId('gamepad')\n"
            "constructorArgumentsPreserved = "
            "deviceId.name == 'gamepad' and deviceId.index == 0\n");

        EXPECT_TRUE(executed);
        EXPECT_TRUE(g_constructorArgumentsPreserved);
    }

    TEST_F(InputDeviceIdReflectionTest, ConstructorWithNameAndIndex_PreservesArgumentsInLua)
    {
        const bool executed = m_scriptContext->Execute(
            "local deviceId = InputDeviceId('gamepad', 3)\n"
            "constructorArgumentsPreserved = "
            "deviceId.name == 'gamepad' and deviceId.index == 3\n");

        EXPECT_TRUE(executed);
        EXPECT_TRUE(g_constructorArgumentsPreserved);
    }
} // namespace InputDeviceIdReflectionTests
