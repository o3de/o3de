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
    Tests/Artifact/TestImpactTargetDescriptorCompilerTest.cpp
    Tests/Artifact/TestImpactBuildTargetDescriptorFactoryTest.cpp
    Tests/Artifact/TestImpactModuleCoverageFactoryTest.cpp
    Tests/Artifact/TestImpactTestEnumerationSuiteFactoryTest.cpp
    Tests/Artifact/TestImpactTestRunSuiteFactoryTest.cpp
    Tests/Artifact/TestImpactTestTargetMetaMapFactoryTest.cpp
    Tests/Artifact/TestImpactDependencyGraphDataFactoryTest.cpp
    Tests/Process/TestImpactProcessSchedulerTest.cpp
    Tests/Dependency/TestImpactDynamicDependencyMapTest.cpp
    Tests/Dependency/TestImpactSourceDependencyTest.cpp
    Tests/Dependency/TestImpactSourceCoveringTestsListTest.cpp
    Tests/Dependency/TestImpactSourceCoveringTestsSerializerTest.cpp
    Tests/Dependency/TestImpactTestSelectorAndPrioritizerTest.cpp
    Tests/Process/TestImpactProcessTest.cpp
    Tests/Target/TestImpactBuildTargetTest.cpp
    Tests/TestImpactExceptionTest.cpp
    Tests/TestImpactFrameworkPathTest.cpp
    Tests/TestEngine/TestImpactTestEnumeratorTest.cpp
    Tests/TestEngine/TestImpactTestEumerationSerializerTest.cpp
    Tests/TestEngine/TestImpactTestRunSerializerTest.cpp
    Tests/TestEngine/TestImpactTestRunnerTest.cpp
    Tests/TestEngine/TestImpactInstrumentedTestRunnerTest.cpp
    Tests/TestEngine/TestImpactTestCoverageTest.cpp
    Tests/TestEngine/TestImpactTestEngineTest.cpp
    Tests/TestEngine/TestImpactTestJobInfoGeneratorTest.cpp
    Tests/TestImpactTestJobRunnerCommon.h
    Tests/TestImpactRuntimeTestMain.cpp
    Tests/TestImpactTestUtils.cpp
    Tests/TestImpactTestUtils.h
    Tests/TestImpactMicroRepo.cpp
    Tests/TestImpactMicroRepo.h
    Tests/TestImpactRuntimeTest.cpp
    Tests/TestImpactChangeListSerializerTest.cpp
)
