/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Common/Run/TestImpactTestCoverage.h>

#include <AzCore/std/string/string.h>

namespace TestImpact
{
    namespace Cobertura
    {
        //! Serializes the specified test coverage to Cobertura format.
        AZStd::string SerializeTestCoverage(const TestCoverage& testCoverage, const RepoPath& repoRoot);
    } // namespace GTest
} // namespace TestImpact
