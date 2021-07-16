/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SystemComponentFixture.h"

namespace EMotionFX
{

TEST_F(SystemComponentFixture, SystemComponentLoads)
{
    ASSERT_TRUE(true);
}

TEST_F(SystemComponentFixture, SystemComponentLoadsAgain)
{
    // Having 2 tests that use the SystemComponentFixture ensures that a
    // subsequent call to SystemComponent::Activate() that comes after a
    // SystemComponent::Deactivate() functions as expected.
    ASSERT_TRUE(true);
}

} // end namespace EMotionFX
