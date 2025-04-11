/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/UnitTest/TestTypes.h>

namespace EMotionFX
{
    class MCoreSystemFixture
        : public UnitTest::LeakDetectionFixture
    {
    public:
        void SetUp() override;
        void TearDown() override;
    };
}
