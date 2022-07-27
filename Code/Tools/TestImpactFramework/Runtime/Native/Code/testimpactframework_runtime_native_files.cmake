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
    Source/Artifact/Factory/TestImpactNativeTargetDescriptorFactory.cpp
    Source/Artifact/Factory/TestImpactNativeTargetDescriptorFactory.h
    Source/Artifact/Factory/TestImpactNativeTestTargetMetaMapFactory.cpp
    Source/Artifact/Factory/TestImpactNativeTestTargetMetaMapFactory.h
    Source/Artifact/Static/TestImpactNativeTargetDescriptor.cpp
    Source/Artifact/Static/TestImpactNativeTargetDescriptor.h
    Source/Artifact/Static/TestImpactNativeTargetDescriptorCompiler.cpp
    Source/Artifact/Static/TestImpactNativeTargetDescriptorCompiler.h
    Source/Artifact/Static/TestImpactNativeProductionTargetDescriptor.cpp
    Source/Artifact/Static/TestImpactNativeProductionTargetDescriptor.h
    Source/Artifact/Static/TestImpactNativeTestTargetMeta.h
    Source/Artifact/Static/TestImpactNativeTestTargetDescriptor.cpp
    Source/Artifact/Static/TestImpactNativeTestTargetDescriptor.h
    Source/Target/Native/TestImpactNativeTarget.cpp
    Source/Target/Native/TestImpactNativeTarget.h
    Source/Target/Native/TestImpactNativeProductionTarget.cpp
    Source/Target/Native/TestImpactNativeProductionTarget.h
    Source/Target/Native/TestImpactNativeTestTarget.cpp
    Source/Target/Native/TestImpactNativeTestTarget.h
    Source/TestRunner/Native/TestImpactNativeInstrumentedTestRunner.h
    Source/TestRunner/Native/TestImpactNativeRegularTestRunner.h
    Source/TestRunner/Native/TestImpactNativeTestEnumerator.h
    Source/TestRunner/Native/Job/TestImpactNativeTestRunJobData.h
    Source/TestEngine/Native/TestImpactNativeTestEngine.cpp
    Source/TestEngine/Native/TestImpactNativeTestEngine.h
    Source/TestEngine/Native/TestImpactNativeErrorCodeChecker.cpp
    Source/TestEngine/Native/TestImpactNativeErrorCodeChecker.h
    Source/TestEngine/Native/TestImpactNativeTestTargetExtension.h
    Source/TestEngine/Native/Job/TestImpactNativeTestJobInfoUtils.cpp
    Source/TestEngine/Native/Job/TestImpactNativeTestJobInfoUtils.h
    Source/TestEngine/Native/Job/TestImpactNativeTestJobInfoGenerator.cpp
    Source/TestEngine/Native/Job/TestImpactNativeTestJobInfoGenerator.h
    Source/TestImpactNativeRuntime.cpp
    Source/TestImpactNativeRuntimeUtils.cpp
    Source/TestImpactNativeRuntimeUtils.h
)
