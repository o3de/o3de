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

#include <Artifact/Static/TestImpactProductionTargetDescriptor.h>
#include <Artifact/Static/TestImpactTestTargetDescriptor.h>
#include <Artifact/Static/TestImpactTestTargetMeta.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/tuple.h>

namespace TestImpact
{
    //! Compiles the production target artifacts and test target artifactss from the supplied build target artifacts and test target meta
    //! map artifact.
    //! @param buildTargets The list of build target artifacts to be sorted into production and test artifact types.
    //! @param testTargetMetaMap The map of test target meta artifacts containing the additional meta-data about each test target.
    //! @return A tuple containing the production artifacts and test artifacts.
    AZStd::tuple<AZStd::vector<ProductionTargetDescriptor>, AZStd::vector<TestTargetDescriptor>> CompileTargetDescriptors(
        AZStd::vector<BuildTargetDescriptor>&& buildTargets, TestTargetMetaMap&& testTargetMetaMap);
} // namespace TestImpact
