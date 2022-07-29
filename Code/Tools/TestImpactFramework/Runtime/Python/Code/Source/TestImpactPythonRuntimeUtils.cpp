/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactPythonRuntimeUtils.h>

namespace TestImpact
{
    AZStd::unique_ptr<BuildTargetList<PythonTestTarget, PythonProductionTarget>> ConstructPythonBuildTargetList(
        SuiteType suiteFilter,
        const BuildTargetDescriptorConfig& buildTargetDescriptorConfig,
        const TestTargetMetaConfig& testTargetMetaConfig)
    {
        auto pythonTestTargetMetaMap = ReadPythonTestTargetMetaMapFile(suiteFilter, testTargetMetaConfig.m_metaFile);
        auto pythonTargetDescriptors = ReadPythonTargetDescriptorFiles(buildTargetDescriptorConfig);
        auto buildTargets = CompileTargetDescriptors(AZStd::move(pythonTargetDescriptors), AZStd::move(pythonTestTargetMetaMap));
        auto&& [productionTargets, testTargets] = buildTargets;
        return AZStd::make_unique<BuildTargetList<PythonTestTarget, PythonProductionTarget>>(
            AZStd::move(testTargets), AZStd::move(productionTargets));
    }
} // namespace TestImpact
