/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactPythonTestTargetDescriptor.h>
#include <TestImpactFramework/TestImpactTestSequence.h>

#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    //! Constructs a build target artifact from the specified build target data.
    //! @param masterTestListData The  master test list data specifying each test target type in the repository.
    //! @param suiteType The test suite to filter for selection from the test target descriptors.
    AZStd::vector<PythonTestTargetDescriptor> PythonTestTargetDescriptorFactory(const AZStd::string& masterTestListData, SuiteType suiteType);
} // namespace TestImpact
