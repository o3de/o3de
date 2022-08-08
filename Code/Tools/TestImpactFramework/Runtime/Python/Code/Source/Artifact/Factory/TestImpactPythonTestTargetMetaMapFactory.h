/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactPythonTestTargetMeta.h>
#include <TestImpactFramework/TestImpactTestSequence.h>

namespace TestImpact
{
    //! Constructs a list of test target meta-data artifacts of the specified suite type from the specified master test list data.
    //! @param testListData The raw test list data in JSON format.
    //! @param suiteType The suite type to select the target meta-data artifacts from.
    //! @return The constructed list of test target meta-data artifacts.
    PythonTestTargetMetaMap PythonTestTargetMetaMapFactory(const AZStd::string& testListData, SuiteType suiteType);
} // namespace TestImpact
