/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Script/LuaEditorSystemComponent.h>

#include <AzCore/Script/ScriptContext.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    class LuaEditorSystemComponentTests
        : public ::testing::Test
    {
    public:
        void SetUp() {}
        void TearDown() {}

        void TestValidateLuaComponentBoilerplate()
        {
            AZ::ScriptContext scriptContext = AZ::ScriptContext();
            AZStd::string luaComponentBoilerplate =
                AzToolsFramework::Script::LuaEditorSystemComponent::GenerateLuaComponentBoilerplate("TEST_SCRIPT");
            ASSERT_TRUE(scriptContext.Execute(luaComponentBoilerplate.c_str()));
        }
    };

    TEST_F(LuaEditorSystemComponentTests, LuaEditorSystemComponentValidateLuaComponentBoilerplate)
    {
        TestValidateLuaComponentBoilerplate();
    }
} // namespace UnitTest
