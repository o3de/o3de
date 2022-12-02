/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/MCoreSystemFixture.h>
#include <MCore/Source/MCoreSystem.h>

namespace EMotionFX
{
    void MCoreSystemFixture::SetUp()
    {
        UnitTest::LeakDetectionFixture::SetUp();
        MCore::Initializer::Init();
    }

    void MCoreSystemFixture::TearDown()
    {
        MCore::Initializer::Shutdown();
        UnitTest::LeakDetectionFixture::TearDown();
    }
}
