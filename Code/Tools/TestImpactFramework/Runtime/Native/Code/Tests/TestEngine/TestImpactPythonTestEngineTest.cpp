/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestEngine/Python/TestImpactPythonTestEngine.h>
#include <TestEngine/Python/Job/TestImpactPythonTestJobInfoGenerator.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    class PythonTestEngineFixture
        : public AllocatorsTestFixture
    {
    public:

    private:

    };

    TEST_F(PythonTestEngineFixture, FOO)
    {
        TestImpact::PythonTestRunJobInfoGenerator generator(
            "E:/o3de_TIF_Feature",
            "E:/o3de_TIF_Feature/python/python.cmd",
            "E:/o3de_TIF_Feature/build/windows_vs2019/bin/debug/",
            "E:/o3de_TIF_Feature/build/windows_vs2019/bin/TestImpactFramework/debug/Temp/PythonRun");

        auto testDescriptor = AZStd::make_unique<TestImpact::PythonTestTargetDescriptor>();
        testDescriptor->m_name = "BlastTests_Main";
        testDescriptor->m_path = "AutomatedTesting/Gem/PythonTests/Blast";
        testDescriptor->m_scriptPath = "AutomatedTesting/Gem/PythonTests/Blast/TestSuite_Main.py";
        testDescriptor->m_testSuiteMeta.m_name = "main";
        TestImpact::PythonTestTarget testTarget(AZStd::move(testDescriptor));

        auto jobInfo = generator.GenerateJobInfo(&testTarget, { 1 });

        AZ_Printf("", jobInfo.GetCommand().m_args.c_str());
    }
} // namespace UnitTest
