/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

#include <AzFramework/Input/Devices/InputDeviceId.h>

#include <AzCore/Math/MathReflection.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace InputCursorReflectionTests
{
    using namespace AzFramework;

    namespace
    {
        // Statics bridged to Lua through reflected properties so the script can hand
        // values back to the test for assertion.
        float g_capturedCursorX = -1.0f;
        float g_capturedCursorY = -1.0f;
        int g_capturedEnum = -1;
        bool g_busVisibleInLua = false;

        // Reflected so the Lua side can extract scalars from the returned Vector2
        // without depending on how Vector2 itself is exposed to script.
        float ExtractVec2X(const AZ::Vector2& v)
        {
            return v.GetX();
        }
        float ExtractVec2Y(const AZ::Vector2& v)
        {
            return v.GetY();
        }

        // Stand-in for the platform mouse backend: answers the cursor requests with a
        // known normalized position so the test can assert what the script reads.
        class StubCursorHandler : public InputSystemCursorRequestBus::Handler
        {
        public:
            void Connect()
            {
                InputSystemCursorRequestBus::Handler::BusConnect(InputDeviceMouse::Id);
            }
            void Disconnect()
            {
                InputSystemCursorRequestBus::Handler::BusDisconnect();
            }

            void SetSystemCursorState(SystemCursorState state) override
            {
                m_state = state;
            }
            SystemCursorState GetSystemCursorState() const override
            {
                return m_state;
            }
            void SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized) override
            {
                m_position = positionNormalized;
            }
            AZ::Vector2 GetSystemCursorPositionNormalized() const override
            {
                return m_position;
            }

            AZ::Vector2 m_position = AZ::Vector2(0.7f, 0.3f);
            SystemCursorState m_state = SystemCursorState::Unknown;
        };
    } // namespace

    class InputDeviceMouseReflectionTest : public UnitTest::LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();

            g_capturedCursorX = -1.0f;
            g_capturedCursorY = -1.0f;
            g_capturedEnum = -1;
            g_busVisibleInLua = false;

            m_behaviorContext = aznew AZ::BehaviorContext();

            // Dependencies the cursor bus binding needs, reflected app-wide in a real
            // launcher: Vector2 (the position type) and InputDeviceId (the bus address,
            // which the Event(id) binding treats as an argument).
            AZ::MathReflect(m_behaviorContext);
            InputDeviceId::Reflect(m_behaviorContext);

            // The reflection under test.
            InputDeviceMouse::Reflect(m_behaviorContext);

            // Test-only helpers/bridges.
            m_behaviorContext->Method("ExtractVec2X", &ExtractVec2X);
            m_behaviorContext->Method("ExtractVec2Y", &ExtractVec2Y);
            m_behaviorContext->Property("g_capturedCursorX", BehaviorValueProperty(&g_capturedCursorX));
            m_behaviorContext->Property("g_capturedCursorY", BehaviorValueProperty(&g_capturedCursorY));
            m_behaviorContext->Property("g_capturedEnum", BehaviorValueProperty(&g_capturedEnum));
            m_behaviorContext->Property("g_busVisibleInLua", BehaviorValueProperty(&g_busVisibleInLua));

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

    // Tier 1: the cursor request bus, its events, and the cursor-state enum are
    // registered in the BehaviorContext by InputDeviceMouse::Reflect.
    TEST_F(InputDeviceMouseReflectionTest, CursorBus_IsReflected_WithEventsAndEnum)
    {
        auto ebusIt = m_behaviorContext->m_ebuses.find("InputSystemCursorRequestBus");
        ASSERT_NE(ebusIt, m_behaviorContext->m_ebuses.end());

        const AZ::BehaviorEBus* ebus = ebusIt->second;
        EXPECT_NE(ebus->m_events.find("GetSystemCursorPositionNormalized"), ebus->m_events.end());
        EXPECT_NE(ebus->m_events.find("SetSystemCursorPositionNormalized"), ebus->m_events.end());
        EXPECT_NE(ebus->m_events.find("GetSystemCursorState"), ebus->m_events.end());
        EXPECT_NE(ebus->m_events.find("SetSystemCursorState"), ebus->m_events.end());

        // Enum<>() reflects each value as a named global property.
        EXPECT_NE(
            m_behaviorContext->m_properties.find("SystemCursorState_ConstrainedAndVisible"),
            m_behaviorContext->m_properties.end());
    }

    // Tier 2: a gameplay-style Lua script can see the bus and the enum, and reading
    // the cursor position dispatches to a connected handler and returns its value.
    // This is the end-to-end proof that "aim at the cursor" is now possible from a
    // game script: the normalized position is readable, ready to feed ScreenNdcToWorld.
    TEST_F(InputDeviceMouseReflectionTest, CursorPosition_ReadableFromLua)
    {
        StubCursorHandler stub;
        stub.Connect();

        const char* script =
            "g_busVisibleInLua = (InputSystemCursorRequestBus ~= nil)\n"
            "g_capturedEnum = SystemCursorState_ConstrainedAndVisible\n"
            "local p = InputSystemCursorRequestBus.Broadcast.GetSystemCursorPositionNormalized()\n"
            "g_capturedCursorX = ExtractVec2X(p)\n"
            "g_capturedCursorY = ExtractVec2Y(p)\n";

        const bool executed = m_scriptContext->Execute(script);

        stub.Disconnect();

        EXPECT_TRUE(executed);
        EXPECT_TRUE(g_busVisibleInLua);
        EXPECT_EQ(g_capturedEnum, static_cast<int>(SystemCursorState::ConstrainedAndVisible));
        EXPECT_NEAR(g_capturedCursorX, 0.7f, 0.0001f);
        EXPECT_NEAR(g_capturedCursorY, 0.3f, 0.0001f);
    }
} // namespace InputCursorReflectionTests
