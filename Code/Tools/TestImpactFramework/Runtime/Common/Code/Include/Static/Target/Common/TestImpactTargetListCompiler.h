/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/TestImpactArtifactException.h>
#include <Artifact/Static/TestImpactTargetDescriptor.h>
#include <Target/Common/TestImpactTargetList.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/tuple.h>

namespace TestImpact
{
    //! Compiles the production target artifacts and test target artifactss from the supplied build target artifacts and test target meta
    //! map artifact.
    //! @param buildTargets The list of build target artifacts to be sorted into production and test artifact types.
    //! @param NativeTestTargetMetaMap The map of test target meta artifacts containing the additional meta-data about each test target.
    //! @return A tuple containing the production artifacts and test artifacts.
    template<typename ProductionTarget, typename TestTarget, typename TestTargetMetaMap>
    AZStd::tuple<TargetList<ProductionTarget>, TargetList<TestTarget>>
    CompileTargetLists(
        AZStd::vector<TargetDescriptor>&& buildTargetDescriptors, TestTargetMetaMap&& testTargetMetaMap)
    {
        AZ_TestImpact_Eval(!buildTargetDescriptors.empty(), ArtifactException, "Build target descriptor list cannot be null");
        AZ_TestImpact_Eval(!testTargetMetaMap.empty(), ArtifactException, "Test target meta map cannot be null");

        AZStd::vector<ProductionTarget> productionTargets;
        AZStd::vector<TestTarget> testTargets;

        for (auto&& descriptor : buildTargetDescriptors)
        {
            // If this build target has an associated test artifact then it is a test target, otherwise it is a production target
            if (auto&& testTargetMeta = testTargetMetaMap.find(descriptor.m_name); testTargetMeta != testTargetMetaMap.end())
            {
                testTargets.emplace_back(AZStd::move(descriptor), AZStd::move(testTargetMeta->second));
            }
            else
            {
                productionTargets.emplace_back(AZStd::move(descriptor));
            }
        }

        return { { AZStd::move(productionTargets) }, { AZStd::move(testTargets) } };
    }
} // namespace TestImpact
