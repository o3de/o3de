#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(FILES
    Include/TestImpactFramework/TestImpactBitwise.h
    Include/TestImpactFramework/TestImpactCallback.h
    Include/TestImpactFramework/TestImpactException.h
    Include/TestImpactFramework/TestImpactFrameworkPath.h
    Source/Artifact/TestImpactArtifactException.h
    Source/Artifact/Factory/TestImpactBuildTargetDescriptorFactory.cpp
    Source/Artifact/Factory/TestImpactBuildTargetDescriptorFactory.h
    Source/Artifact/Factory/TestImpactChangeListFactory.cpp
    Source/Artifact/Factory/TestImpactChangeListFactory.h
    Source/Artifact/Factory/TestImpactTestEnumerationSuiteFactory.cpp
    Source/Artifact/Factory/TestImpactTestEnumerationSuiteFactory.h
    Source/Artifact/Factory/TestImpactTestRunSuiteFactory.cpp
    Source/Artifact/Factory/TestImpactTestRunSuiteFactory.h
    Source/Artifact/Factory/TestImpactTestTargetMetaMapFactory.cpp
    Source/Artifact/Factory/TestImpactTestTargetMetaMapFactory.h
    Source/Artifact/Factory/TestImpactModuleCoverageFactory.cpp
    Source/Artifact/Factory/TestImpactModuleCoverageFactory.h
    Source/Artifact/Static/TestImpactBuildTargetDescriptor.cpp
    Source/Artifact/Static/TestImpactBuildTargetDescriptor.h
    Source/Artifact/Static/TestImpactTargetDescriptorCompiler.cpp
    Source/Artifact/Static/TestImpactTargetDescriptorCompiler.h
    Source/Artifact/Static/TestImpactProductionTargetDescriptor.cpp
    Source/Artifact/Static/TestImpactProductionTargetDescriptor.h
    Source/Artifact/Static/TestImpactTestTargetMeta.h
    Source/Artifact/Static/TestImpactTestTargetDescriptor.cpp
    Source/Artifact/Static/TestImpactTestTargetDescriptor.h
    Source/Artifact/Dynamic/TestImpactChangelist.h
    Source/Artifact/Dynamic/TestImpactTestEnumerationSuite.h
    Source/Artifact/Dynamic/TestImpactTestRunSuite.h
    Source/Artifact/Dynamic/TestImpactTestSuite.h
    Source/Artifact/Dynamic/TestImpactCoverage.h
    Source/Process/TestImpactProcess.cpp
    Source/Process/TestImpactProcess.h
    Source/Process/TestImpactProcessException.h
    Source/Process/TestImpactProcessInfo.cpp
    Source/Process/TestImpactProcessInfo.h
    Source/Process/TestImpactProcessLauncher.h
    Source/Process/JobRunner/TestImpactProcessJob.h
    Source/Process/JobRunner/TestImpactProcessJobInfo.h
    Source/Process/JobRunner/TestImpactProcessJobRunner.h
    Source/Process/Scheduler/TestImpactProcessScheduler.cpp
    Source/Process/Scheduler/TestImpactProcessScheduler.h

    Source/Target/TestImpactBuildTarget.cpp
    Source/Target/TestImpactBuildTarget.h
    Source/Target/TestImpactBuildTargetList.h
    Source/Target/TestImpactProductionTarget.cpp
    Source/Target/TestImpactProductionTarget.h
    Source/Target/TestImpactProductionTargetList.h
    Source/Target/TestImpactTargetException.h
    Source/Target/TestImpactTestTarget.cpp
    Source/Target/TestImpactTestTarget.h
    Source/Target/TestImpactTestTargetList.h
    Source/Test/Enumeration/TestImpactTestEnumeration.h
    Source/Test/Enumeration/TestImpactTestEnumerationException.h
    Source/Test/Enumeration/TestImpactTestEnumerationSerializer.cpp
    Source/Test/Enumeration/TestImpactTestEnumerationSerializer.h
    Source/Test/Enumeration/TestImpactTestEnumerator.cpp
    Source/Test/Enumeration/TestImpactTestEnumerator.h
    Source/Test/Run/TestImpactTestRunSerializer.cpp
    Source/Test/Run/TestImpactTestRunSerializer.h
    Source/Test/Run/TestImpactTestRunner.cpp
    Source/Test/Run/TestImpactTestRunner.h
    Source/Test/Run/TestImpactInstrumentedTestRunner.cpp
    Source/Test/Run/TestImpactInstrumentedTestRunner.h
    Source/Test/Run/TestImpactTestRun.cpp
    Source/Test/Run/TestImpactTestRun.h
    Source/Test/Run/TestImpactTestRunJobData.cpp
    Source/Test/Run/TestImpactTestRunJobData.h
    Source/Test/Run/TestImpactTestCoverage.cpp
    Source/Test/Run/TestImpactTestCoverage.h
    Source/Test/Run/TestImpactTestRunException.h
    Source/Test/Job/TestImpactTestJobRunner.h
    Source/Test/Job/TestImpactTestJobException.h
    Source/Test/Job/TestImpactTestJobCommon.h
    Source/Test/TestImpactTestSuiteContainer.h
    Source/TestImpactException.cpp
    Source/TestImpactFrameworkPath.cpp
)
