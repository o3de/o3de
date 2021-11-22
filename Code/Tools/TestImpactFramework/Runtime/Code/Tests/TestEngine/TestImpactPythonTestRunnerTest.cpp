/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestEngine/Python/Run/TestImpactPythonTestRunner.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    class PyTestRunnerFixture
        : public AllocatorsTestFixture
    {
    public:
        PyTestRunnerFixture()
        {
        }

        TestImpact::PythonTestRunner m_testRunner;
    };

    //TEST_F(PyTestRunnerFixture, Foo)
    //{
    //    try
    //    {
    //        using JobInfo = TestImpact::PythonTestRunner::JobInfo;
    //        using JobData = TestImpact::PythonTestRunner::JobData;
    //        using Command = TestImpact::PythonTestRunner::Command;
//
    //        const AZStd::vector<AZStd::string> cmds =
    //        {
    //             /*"E:/o3de_TIF_Feature/python/python.cmd -m pytest -s E:/o3de_TIF_Feature/AutomatedTesting/Gem/PythonTests/Atom/TestSuite_Main.py  --build-directory E:/o3de_TIF_Feature/build/windows_vs2019/bin/debug/ --junitxml=e:/Atom_TestSuite_Main.xml"
    //            ,"E:/o3de_TIF_Feature/python/python.cmd -m pytest -s E:/o3de_TIF_Feature/AutomatedTesting/Gem/PythonTests/Atom/TestSuite_Main_GPU.py  --build-directory E:/o3de_TIF_Feature/build/windows_vs2019/bin/debug/ --junitxml=e:/Atom_TestSuite_Main_GPU.xml"
    //            ,"E:/o3de_TIF_Feature/python/python.cmd -m pytest -s E:/o3de_TIF_Feature/AutomatedTesting/Gem/PythonTests/assetpipeline/fbx_tests/pythonassetbuildertests.py  --build-directory E:/o3de_TIF_Feature/build/windows_vs2019/bin/debug/ --junitxml=e:/SceneProcessingTests.PythonAssetBuilderTests.xml"
    //            ,"E:/o3de_TIF_Feature/python/python.cmd -m pytest -s E:/o3de_TIF_Feature/AutomatedTesting/Gem/PythonTests/physics/TestSuite_Main.py  --build-directory E:/o3de_TIF_Feature/build/windows_vs2019/bin/debug/ --junitxml=e:/PhysicsTests_Main.xml"
    //            ,"E:/o3de_TIF_Feature/python/python.cmd -m pytest -s E:/o3de_TIF_Feature/AutomatedTesting/Gem/PythonTests/WhiteBox/TestSuite_Main.py  --build-directory E:/o3de_TIF_Feature/build/windows_vs2019/bin/debug/ --junitxml=e:/WhiteBoxTests.xml"
    //            ,"E:/o3de_TIF_Feature/python/python.cmd -m pytest -s E:/o3de_TIF_Feature/AutomatedTesting/Gem/PythonTests/NvCloth/TestSuite_Main.py  --build-directory E:/o3de_TIF_Feature/build/windows_vs2019/bin/debug/ --junitxml=e:/NvClothTests_Main.xml"
    //            ,"E:/o3de_TIF_Feature/python/python.cmd -m pytest -s E:/o3de_TIF_Feature/AutomatedTesting/Gem/PythonTests/prefab/TestSuite_Main.py  --build-directory E:/o3de_TIF_Feature/build/windows_vs2019/bin/debug/ --junitxml=e:/PrefabTests.xml"
    //            ,"E:/o3de_TIF_Feature/python/python.cmd -m pytest -s E:/o3de_TIF_Feature/AutomatedTesting/Gem/PythonTests/Blast/TestSuite_Main.py  --build-directory E:/o3de_TIF_Feature/build/windows_vs2019/bin/debug/ --junitxml=e:/BlastTests_Main.xml"
    //            ,*/"E:/o3de_TIF_Feature/python/python.cmd -m pytest -s E:/o3de_TIF_Feature/AutomatedTesting/Gem/PythonTests/largeworlds/dyn_veg/TestSuite_Main.py  --build-directory E:/o3de_TIF_Feature/build/windows_vs2019/bin/debug/ --junitxml=e:/DynamicVegetationTests_Main.xml"
    //            //,"E:/o3de_TIF_Feature/python/python.cmd -m pytest -s E:/o3de_TIF_Feature/AutomatedTesting/Gem/PythonTests/largeworlds/landscape_canvas/TestSuite_Main.py  --build-directory E:/o3de_TIF_Feature/build/windows_vs2019/bin/debug/ --junitxml=e:/LandscapeCanvasTests_Main.xml"
    //            //,"E:/o3de_TIF_Feature/python/python.cmd -m pytest -s E:/o3de_TIF_Feature/AutomatedTesting/Gem/PythonTests/editor/TestSuite_Main.py  --build-directory E:/o3de_TIF_Feature/build/windows_vs2019/bin/debug/ --junitxml=e:/EditorTests_Main.xml"
    //            //,"E:/o3de_TIF_Feature/python/python.cmd -m pytest -s E:/o3de_TIF_Feature/AutomatedTesting/Gem/PythonTests/editor/TestSuite_Main.py  --build-directory E:/o3de_TIF_Feature/build/windows_vs2019/bin/debug/ --junitxml=e:/EditorTests_Main_GPU.xml"
    //            //,"E:/o3de_TIF_Feature/python/python.cmd -m pytest -s E:/o3de_TIF_Feature/AutomatedTesting/Gem/PythonTests/smoke/test_Editor_NewExistingLevels_Works.py  --build-directory E:/o3de_TIF_Feature/build/windows_vs2019/bin/debug/ --junitxml=e:/EditorTestWithGPU.xml"
    //            //,"E:/o3de_TIF_Feature/python/python.cmd -m pytest -s E:/o3de_TIF_Feature/Gems/Atom/RPI/Tools/atom_rpi_tools/tests  --build-directory E:/o3de_TIF_Feature/build/windows_vs2019/bin/debug/ --junitxml=e:/atom_rpi_tools_tests.xml"
    //        };
//
    //        //JobData jobData("");
    //        //const AZStd::vector<JobInfo> jobInfos =
    //        //{ JobInfo({ 1 }, {"E:\\o3de_TIF_Feature\\python\\python.cmd -m pytest E:\\o3de_TIF_Feature\\AutomatedTesting\\Gem\\PythonTests\\Physics\\TestSuite_Main.py  --build-directory E:\\o3de_TIF_Feature\\build\\windows_vs2019\\bin\\debug\\ --junitxml=e:\\PI_report_py.xml"}, AZStd::move(jobData))
    //        //};
//
    //        AZStd::vector<JobInfo> jobInfos;
    //        size_t id = 0;
    //        for (const auto& cmd : cmds)
    //        {
    //            id++;
    //            jobInfos.push_back(JobInfo({ id }, { cmd }, JobData("e:/DynamicVegetationTests_Main.xml", "E:/o3de_TIF_Feature/build/windows_vs2019/bin/TestImpactFramework/debug/Temp/RuntimeArtifact")));
    //        }
//
    //       //jobInfos.push_back(JobInfo(
    //       //    { 1 },
    //       //    { "E:/o3de_TIF_Feature/python/python.cmd -m pytest "
    //       //      "E:/o3de_TIF_Feature/AutomatedTesting/Gem/PythonTests/assetpipeline/fbx_tests/pythonassetbuildertests.py  "
    //       //      "--build-directory E:/o3de_TIF_Feature/build/windows_vs2019/bin/debug/ "
    //       //      "--junitxml=e:/SceneProcessingTests.PythonAssetBuilderTests.xml" },
    //       //    JobData("e:/SceneProcessingTests.PythonAssetBuilderTests.xml")));
//
    //        auto [result, runnerJobs] = m_testRunner.RunTests(
    //            jobInfos,
    //            TestImpact::StdOutputRouting::ToParent,
    //            TestImpact::StdErrorRouting::None,
    //            AZStd::nullopt,
    //            AZStd::nullopt,
    //            []([[maybe_unused]] const JobInfo& jobInfo, [[maybe_unused]] const TestImpact::JobMeta& meta,
    //                [[maybe_unused]] TestImpact::StdContent&& std)
    //            {
    //                printf("RET: %i, CMD: %s\n", meta.m_returnCode.value_or(-10000), jobInfo.GetCommand().m_args.c_str());
    //                return TestImpact::ProcessCallbackResult::Continue;
    //            },
    //            []([[maybe_unused]] const JobInfo& jobInfo, [[maybe_unused]] const AZStd::string& stdOutput,
    //                [[maybe_unused]] const AZStd::string& stdError,
    //                AZStd::string&& stdOutDelta,
    //                [[maybe_unused]] AZStd::string&& stdErrDelta)
    //            {
    //                printf("%s", stdOutDelta.c_str());
    //                return TestImpact::ProcessCallbackResult::Continue;
    //            });
    //    }
    //    catch(const std::exception& e)
    //    {
    //        printf("%s", e.what());
    //    }
//
    //    return;
    //}
}
