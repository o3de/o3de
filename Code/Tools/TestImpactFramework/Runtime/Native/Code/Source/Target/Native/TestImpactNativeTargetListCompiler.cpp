/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Artifact/TestImpactArtifactException.h>
#include <Target/Native/TestImpactNativeTargetListCompiler.h>

namespace TestImpact
{
    //! Compiles the production target artifacts and test target artifactss from the supplied build target artifacts and test target meta
    //! map artifact.
    //! @param buildTargets The list of build target artifacts to be sorted into production and test artifact types.
    //! @param NativeTestTargetMetaMap The map of test target meta artifacts containing the additional meta-data about each test target.
    //! @return A tuple containing the production artifacts and test artifacts.
    AZStd::tuple<TargetList<NativeProductionTarget>, TargetList<NativeTestTarget>> CompileNativeTargetLists(
        AZStd::vector<TargetDescriptor>&& buildTargetDescriptors, NativeTestTargetMetaMap&& testTargetMetaMap)
    {
        AZ_TestImpact_Eval(!buildTargetDescriptors.empty(), ArtifactException, "Build target descriptor list cannot be null");
        AZ_TestImpact_Eval(!testTargetMetaMap.empty(), ArtifactException, "Test target meta map cannot be null");

        AZStd::vector<NativeProductionTarget> productionTargets;
        AZStd::vector<NativeTestTarget> testTargets;

        size_t numDiscardedTestTargets = 0;
        for (auto&& descriptor : buildTargetDescriptors)
        {
            if (auto&& testTargetMeta = testTargetMetaMap.find(descriptor.m_name); testTargetMeta != testTargetMetaMap.end())
            {
                AZ_TestImpact_Eval(descriptor.m_type == TargetType::TestTarget, ArtifactException, "Target has associated target meta but is a production target");
                testTargets.emplace_back(AZStd::move(descriptor), AZStd::move(testTargetMeta->second));
            }
            else if (descriptor.m_type == TargetType::ProductionTarget)
            {
                productionTargets.emplace_back(AZStd::move(descriptor));
            }
            else
            {
                // Test targets without an associated test target meta artifact are either not opted in to TIAF or not in the
                // suite(s) to be run so will be skipped
                numDiscardedTestTargets++;
            }
        }

        if (numDiscardedTestTargets)
        {
            AZ_Info(
                "CompileNativeTargetLists",
                "%zu test targets were either opted-out of TIAF or not in the suite(s) selected for this run.\n",
                numDiscardedTestTargets);
        }

        return { { AZStd::move(productionTargets) }, { AZStd::move(testTargets) } };
    }
} // namespace TestImpact
