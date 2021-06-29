/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Dynamic/TestImpactTestEnumerationSuite.h>
#include <TestEngine/TestImpactTestSuiteContainer.h>

namespace TestImpact
{
    //! Representation of a given test target's enumerated tests.
    using TestEnumeration = TestSuiteContainer<TestEnumerationSuite>;
} // namespace TestImpact
