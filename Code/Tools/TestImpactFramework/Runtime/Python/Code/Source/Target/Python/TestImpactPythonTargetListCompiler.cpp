/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Artifact/TestImpactArtifactException.h>
#include <Target/Python/TestImpactPythonTargetListCompiler.h>

namespace TestImpact
{
    AZStd::tuple<TargetList<PythonProductionTarget>, TargetList<PythonTestTarget>> CompilePythonTargetLists(
        AZStd::vector<TargetDescriptor>&& buildTargetDescriptors, PythonTestTargetMetaMap&& testTargetMetaMap)
    {
        AZ_TestImpact_Eval(!buildTargetDescriptors.empty(), ArtifactException, "Build target descriptor list cannot be null");
        AZ_TestImpact_Eval(!testTargetMetaMap.empty(), ArtifactException, "Test target meta map cannot be null");

        AZStd::vector<PythonProductionTarget> productionTargets;
        AZStd::vector<PythonTestTarget> testTargets;

        for (auto&& descriptor : buildTargetDescriptors)
        {
            if(descriptor.m_type != TargetType::TestTarget)
            {
                // Python test targets are compiled and added programmatically in the steps that follow so ignore any discovered test targets
                productionTargets.emplace_back(AZStd::move(descriptor));
            }
        }

        for (auto&& testTargetMeta : testTargetMetaMap)
        {
            TargetDescriptor descriptor;
            descriptor.m_name = testTargetMeta.first;
            descriptor.m_type = TargetType::ProductionTarget;
            descriptor.m_sources.m_staticSources.push_back(testTargetMeta.second.m_scriptMeta.m_scriptPath);

            testTargets.emplace_back(AZStd::move(descriptor), AZStd::move(testTargetMeta.second));
        }

        return { { AZStd::move(productionTargets) }, { AZStd::move(testTargets) } };
    }
} // namespace TestImpact
