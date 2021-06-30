/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Dynamic/TestImpactTestSuite.h>

namespace TestImpact
{
    using TestEnumerationCase = TestCase; //!< Test case for test enumeration artifacts.
    using TestEnumerationSuite = TestSuite<TestEnumerationCase>; //!< Test suite for test enumeration artifacts.
} // namespace TestImpact
