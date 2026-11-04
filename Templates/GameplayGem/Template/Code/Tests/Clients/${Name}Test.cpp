// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Components/${SanitizedCppName}ComponentController.h>

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}Test
        : public UnitTest::AllocatorsTestFixture
    {
    };

    TEST_F(${SanitizedCppName}Test, SpeedMultiplier_DefaultValue_MatchesConfig)
    {
        ${SanitizedCppName}ComponentConfig config;
        config.m_speedMultiplier = 2.5f;

        ${SanitizedCppName}ComponentController controller(config);
        EXPECT_FLOAT_EQ(controller.GetSpeedMultiplier(), 2.5f);
    }

    TEST_F(${SanitizedCppName}Test, IsEnabled_DefaultValue_IsTrue)
    {
        ${SanitizedCppName}ComponentConfig config;
        ${SanitizedCppName}ComponentController controller(config);
        EXPECT_TRUE(controller.IsEnabled());
    }
} // namespace ${SanitizedCppName}
