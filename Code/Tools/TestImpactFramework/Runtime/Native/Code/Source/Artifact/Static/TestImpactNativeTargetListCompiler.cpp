/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Artifact/Static/TestImpactNativeTargetListCompiler.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/std/containers/unordered_map.h>

namespace TestImpact
{
    AZStd::tuple<TargetList<NativeProductionTarget>, TargetList<NativeTestTarget>> CompileTargetLists(
        AZStd::vector<AZStd::tuple<TargetDescriptor, NativeTargetDescriptor>>&& buildTargetDescriptors,
        NativeTestTargetMetaMap&& nativeTestTargetMetaMap)
    {
        AZ_TestImpact_Eval(!buildTargetDescriptors.empty(), ArtifactException, "Build target descriptor list cannot be null");
        AZ_TestImpact_Eval(!nativeTestTargetMetaMap.empty(), ArtifactException, "Test target meta map cannot be null");

        AZStd::vector<NativeProductionTarget> productionTargets;
        AZStd::vector<NativeTestTarget> testTargets;

        for (auto&& buildTargetDescriptor : buildTargetDescriptors)
        {
            auto&& [descriptor, nativeDescriptor] = buildTargetDescriptor;

            // If this build target has an associated test artifact then it is a test target, otherwise it is a production target
            if (auto&& testTargetMeta = nativeTestTargetMetaMap.find(descriptor.m_name); testTargetMeta != nativeTestTargetMetaMap.end())
            {
                testTargets.emplace_back(
                    AZStd::move(descriptor),
                    AZStd::move(nativeDescriptor),
                    AZStd::move(testTargetMeta->second));
            }
            else
            {
                productionTargets.emplace_back(AZStd::move(descriptor), AZStd::move(nativeDescriptor));
            }
        }

        return { { AZStd::move(productionTargets) }, { AZStd::move(testTargets) } };
    }
} // namespace TestImpact
