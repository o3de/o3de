#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Include/TestImpactFramework/Native/TestImpactNativeRuntime.h
    Include/TestImpactFramework/Native/TestImpactNativeConfiguration.h
    Include/TestImpactFramework/Native/TestImpactNativeRuntimeConfigurationFactory.h
    Source/Artifact/Factory/TestImpactNativeTestTargetMetaMapFactory.cpp
    Source/Artifact/Factory/TestImpactNativeTestTargetMetaMapFactory.h
    Source/Artifact/Static/TestImpactNativeTestTargetMeta.h
    Source/Target/Native/TestImpactNativeProductionTarget.h
    Source/Target/Native/TestImpactNativeTestTarget.cpp
    Source/Target/Native/TestImpactNativeTestTarget.h
    Source/Target/Native/TestImpactNativeTargetListCompiler.cpp
    Source/Target/Native/TestImpactNativeTargetListCompiler.h
    Source/TestRunner/Native/TestImpactNativeErrorCodeChecker.cpp
    Source/TestRunner/Native/TestImpactNativeErrorCodeChecker.h
    Source/TestRunner/Native/TestImpactNativeInstrumentedTestRunner.h
    Source/TestRunner/Native/TestImpactNativeRegularTestRunner.h
    Source/TestRunner/Native/TestImpactNativeShardedInstrumentedTestRunner.cpp
    Source/TestRunner/Native/TestImpactNativeShardedInstrumentedTestRunner.h
    Source/TestRunner/Native/TestImpactNativeShardedRegularTestRunner.cpp
    Source/TestRunner/Native/TestImpactNativeShardedRegularTestRunner.h
    Source/TestRunner/Native/TestImpactNativeTestEnumerator.h
    Source/TestRunner/Native/Shard/TestImpactNativeShardedTestJob.h
    Source/TestRunner/Native/Shard/TestImpactNativeShardedTestRunnerBase.h
    Source/TestRunner/Native/Job/TestImpactNativeTestJobInfoGenerator.cpp
    Source/TestRunner/Native/Job/TestImpactNativeTestJobInfoGenerator.h
    Source/TestRunner/Native/Job/TestImpactNativeTestJobInfoUtils.cpp
    Source/TestRunner/Native/Job/TestImpactNativeTestJobInfoUtils.h
    Source/TestRunner/Native/Job/TestImpactNativeTestRunJobData.h
    Source/TestRunner/Native/Job/TestImpactNativeShardedTestJobInfoGenerator.cpp
    Source/TestRunner/Native/Job/TestImpactNativeShardedTestJobInfoGenerator.h
    Source/TestEngine/Native/TestImpactNativeTestEngine.cpp
    Source/TestEngine/Native/TestImpactNativeTestEngine.h
    Source/TestEngine/Native/TestImpactNativeTestTargetExtension.h
    Source/TestImpactNativeRuntime.cpp
    Source/TestImpactNativeRuntimeConfigurationFactory.cpp
)
