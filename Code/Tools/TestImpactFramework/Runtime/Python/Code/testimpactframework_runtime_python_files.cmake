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
    Include/TestImpactFramework/Python/TestImpactPythonRuntimeConfigurationFactory.h
    Source/Artifact/Factory/TestImpactPythonTestTargetMetaMapFactory.cpp
    Source/Artifact/Factory/TestImpactPythonTestTargetMetaMapFactory.h
    Source/Artifact/Static/TestImpactPythonTestTargetMeta.h
    Source/Dependency/TestImpactPythonTestSelectorAndPrioritizer.h
    Source/Target/Python/TestImpactPythonProductionTarget.h
    Source/Target/Python/TestImpactPythonTestTarget.cpp
    Source/Target/Python/TestImpactPythonTestTarget.h
    Source/Target/Python/TestImpactPythonTargetListCompiler.cpp
    Source/Target/Python/TestImpactPythonTargetListCompiler.h
    Source/TestRunner/Python/TestImpactPythonErrorCodeChecker.cpp
    Source/TestRunner/Python/TestImpactPythonErrorCodeChecker.h
    Source/TestRunner/Python/TestImpactPythonInstrumentedTestRunnerBase.cpp
    Source/TestRunner/Python/TestImpactPythonInstrumentedTestRunnerBase.h
    Source/TestRunner/Python/TestImpactPythonInstrumentedTestRunner.h
    Source/TestRunner/Python/TestImpactPythonInstrumentedNullTestRunner.cpp
    Source/TestRunner/Python/TestImpactPythonInstrumentedNullTestRunner.h
    Source/TestRunner/Python/TestImpactPythonRegularTestRunnerBase.cpp
    Source/TestRunner/Python/TestImpactPythonRegularTestRunnerBase.h
    Source/TestRunner/Python/TestImpactPythonRegularTestRunner.h
    Source/TestRunner/Python/TestImpactPythonRegularNullTestRunner.cpp
    Source/TestRunner/Python/TestImpactPythonRegularNullTestRunner.h
    Source/TestRunner/Python/Job/TestImpactPythonTestJobInfoGenerator.cpp
    Source/TestRunner/Python/Job/TestImpactPythonTestJobInfoGenerator.h
    Source/TestEngine/Python/TestImpactPythonTestEngine.cpp
    Source/TestEngine/Python/TestImpactPythonTestEngine.h
    Source/TestImpactPythonRuntime.cpp
    Source/TestImpactPythonRuntimeConfigurationFactory.cpp
)

# Remove this file from unity builds because with VS2019, the include of AzCore/std/string/regex.h can sometimes
# trigger an invalid warning about a mismatched #pragma warning(push) in xlocinfo.h.
list(APPEND SKIP_UNITY_BUILD_INCLUSION_FILES Source/TestImpactPythonRuntime.cpp)
