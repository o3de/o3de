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

#include <Artifact/Static/TestImpactTargetDescriptorCompiler.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/std/containers/unordered_map.h>

namespace TestImpact
{
    AZStd::tuple<AZStd::vector<ProductionTargetDescriptor>, AZStd::vector<TestTargetDescriptor>> CompileTargetDescriptors(
        AZStd::vector<BuildTargetDescriptor>&& buildTargets, TestTargetMetaMap&& testTargetMetaMap)
    {
        AZ_TestImpact_Eval(!buildTargets.empty(), ArtifactException, "Build target descriptor list cannot be null");
        AZ_TestImpact_Eval(!testTargetMetaMap.empty(), ArtifactException, "Test target meta map cannot be null");

        AZStd::tuple<AZStd::vector<ProductionTargetDescriptor>, AZStd::vector<TestTargetDescriptor>> outputTargets;
        auto& [productionTargets, testTargets] = outputTargets;

        for (auto&& buildTarget : buildTargets)
        {
            // If this build target has an associated test artifact then it is a test target, otherwise it is a production target
            if (auto&& testTargetMeta = testTargetMetaMap.find(buildTarget.m_buildMetaData.m_name);
                testTargetMeta != testTargetMetaMap.end())
            {
                testTargets.emplace_back(TestTargetDescriptor(AZStd::move(buildTarget), AZStd::move(testTargetMeta->second)));
            }
            else
            {
                productionTargets.emplace_back(ProductionTargetDescriptor(AZStd::move(buildTarget)));
            }
        }

        return outputTargets;
    }
} // namespace TestImpact
