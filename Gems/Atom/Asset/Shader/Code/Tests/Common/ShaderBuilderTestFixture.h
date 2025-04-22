/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    /**
     * Unit test fixture for setting up memory allocation pools and the AZ::Name dictionary.
     * In the future will be extended as needed.
     */
    class ShaderBuilderTestFixture
        : public LeakDetectionFixture
    {
    protected:
        ///////////////////////////////////////////////////////////////////////
        // LeakDetectionFixture overrides
        void SetUp() override;
        void TearDown() override;
        ///////////////////////////////////////////////////////////////////////

    };
} // namespace UnitTest

