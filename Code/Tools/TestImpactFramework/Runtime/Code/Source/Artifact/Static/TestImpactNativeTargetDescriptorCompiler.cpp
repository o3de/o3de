/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Artifact/Static/TestImpactNativeTargetDescriptorCompiler.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/std/containers/unordered_map.h>

namespace TestImpact
{
    AZStd::tuple<AZStd::vector<AZStd::unique_ptr<NativeProductionTargetDescriptor>>, AZStd::vector<AZStd::unique_ptr<NativeTestTargetDescriptor>>> CompileTargetDescriptors(
        AZStd::vector<NativeTargetDescriptor>&& buildTargets, NativeTestTargetMetaMap&& NativeTestTargetMetaMap)
    {
        AZ_TestImpact_Eval(!buildTargets.empty(), ArtifactException, "Build target descriptor list cannot be null");
        AZ_TestImpact_Eval(!NativeTestTargetMetaMap.empty(), ArtifactException, "Test target meta map cannot be null");

        AZStd::tuple<AZStd::vector<AZStd::unique_ptr<NativeProductionTargetDescriptor>>, AZStd::vector<AZStd::unique_ptr<NativeTestTargetDescriptor>>>
            outputTargets;
        auto& [productionTargets, testTargets] = outputTargets;

        for (auto&& buildTarget : buildTargets)
        {
            // If this build target has an associated test artifact then it is a test target, otherwise it is a production target
            if (auto&& testTargetMeta = NativeTestTargetMetaMap.find(buildTarget.m_name);
                testTargetMeta != NativeTestTargetMetaMap.end())
            {
                testTargets.emplace_back(AZStd::make_unique<NativeTestTargetDescriptor>(AZStd::move(buildTarget), AZStd::move(testTargetMeta->second)));
            }
            else
            {
                productionTargets.emplace_back(AZStd::make_unique<NativeProductionTargetDescriptor>(AZStd::move(buildTarget)));
            }
        }

        return outputTargets;
    }
} // namespace TestImpact
