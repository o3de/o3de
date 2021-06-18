/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
