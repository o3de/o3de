/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactTestSequence.h>
#include <Artifact/Static/TestImpactTestTargetMeta.h>

#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    //! Constructs a list of test target meta-data artifacts of the specified suite type from the specified master test list data.
    //! @param masterTestListData The raw master test list data in JSON format.
    //! @param suiteType The suite type to select the target meta-data artifacts from.
    //! @return The constructed list of test target meta-data artifacts.
    TestTargetMetaMap TestTargetMetaMapFactory(const AZStd::string& masterTestListData, SuiteType suiteType);
} // namespace TestImpact
