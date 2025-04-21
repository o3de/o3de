/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactTestSequence.h>
#include <TestImpactFramework/Native/TestImpactNativeConfiguration.h>

#include <Artifact/Static/TestImpactNativeTestTargetMeta.h>

namespace TestImpact
{
    //! Constructs a list of test target meta-data artifacts of the specified suite type from the specified master test list data.
    //! @param masterTestListData The raw master test list data in JSON format.
    //! @param suiteSet The suites to select the target meta-data artifacts from.
    //! @param suiteLabelExcludeSet Any tests with suites that match a label from this set will be excluded.
    //! @return The constructed list of test target meta-data artifacts.
    NativeTestTargetMetaMap NativeTestTargetMetaMapFactory(
        const AZStd::string& masterTestListData,
        const SuiteSet& suiteSet,
        const SuiteLabelExcludeSet& suiteLabelExcludeSet);
} // namespace TestImpact
