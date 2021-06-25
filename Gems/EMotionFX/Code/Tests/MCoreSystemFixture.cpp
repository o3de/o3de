/*
 * Copyright (c) Contributors to the Open 3D Engine Project
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
        UnitTest::AllocatorsTestFixture::SetUp();
        MCore::Initializer::Init();
    }

    void MCoreSystemFixture::TearDown()
    {
        MCore::Initializer::Shutdown();
        UnitTest::AllocatorsTestFixture::TearDown();
    }
}
