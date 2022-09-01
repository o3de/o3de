#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Include/TestImpactFramework/Python/TestImpactPythonRuntime.h
    Include/TestImpactFramework/Python/TestImpactPythonConfiguration.h
    Source/Artifact/Factory/TestImpactPythonTestTargetMetaMapFactory.cpp
    Source/Artifact/Factory/TestImpactPythonTestTargetMetaMapFactory.h
    Source/Artifact/Static/TestImpactPythonTestTargetMeta.h
    Source/Dependency/TestImpactPythonTestSelectorAndPrioritizer.h
    Source/Target/Python/TestImpactPythonProductionTarget.h
    Source/Target/Python/TestImpactPythonTestTarget.cpp
    Source/Target/Python/TestImpactPythonTestTarget.h
    Source/Target/Python/TestImpactPythonTargetListCompiler.cpp
    Source/Target/Python/TestImpactPythonTargetListCompiler.h
    Source/TestRunner/Python/TestImpactPythonTestRunnerBase.cpp
    Source/TestRunner/Python/TestImpactPythonTestRunnerBase.h
    Source/TestRunner/Python/TestImpactPythonTestRunner.h
    Source/TestRunner/Python/TestImpactPythonNullTestRunner.cpp
    Source/TestRunner/Python/TestImpactPythonNullTestRunner.h
    Source/TestRunner/Python/Job/TestImpactPythonTestJobInfoGenerator.cpp
    Source/TestRunner/Python/Job/TestImpactPythonTestJobInfoGenerator.h
    Source/TestEngine/Python/TestImpactPythonTestEngine.cpp
    Source/TestEngine/Python/TestImpactPythonTestEngine.h
    Source/TestEngine/Python/TestImpactPythonErrorCodeChecker.cpp
    Source/TestEngine/Python/TestImpactPythonErrorCodeChecker.h
    Source/TestImpactPythonRuntime.cpp
)
