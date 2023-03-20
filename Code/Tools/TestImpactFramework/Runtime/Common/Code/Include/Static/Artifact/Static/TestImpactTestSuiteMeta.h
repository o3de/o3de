/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Set for the labels appended to each test target's suite.
    using SuiteLabelSet = AZStd::unordered_set<AZStd::string>;

    //! Artifact produced by the build system for each test target containing the additional meta-data about the test.
    struct TestSuiteMeta
    {
        AZStd::string m_name; //!< The name of the test suite.
        AZStd::chrono::milliseconds m_timeout = AZStd::chrono::milliseconds{ 0 }; //!< The timeout for the test suite time to run in.
        SuiteLabelSet m_labelSet; //!< The set of labels for this suite.
    };
} // namespace TestImpact
