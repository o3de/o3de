/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactMicroRepo.h>

namespace UnitTest
{
    namespace MicroRepo
    {
        TestImpact::BuildTargetDescriptor CreateBuildTargetDescriptor(
            const AZStd::string& name, const AZStd::vector<TestImpact::RepoPath>& staticSources, const TestImpact::AutogenSources& autogenSources = {})
        {
            TestImpact::BuildMetaData buildMetaData;
            buildMetaData.m_name = name;
            TestImpact::TargetSources targetSources;
            targetSources.m_staticSources = staticSources;
            targetSources.m_autogenSources = autogenSources;
            for (const auto& autogenSource : autogenSources)
            {
                for (const auto& autogenOutput : autogenSource.m_outputs)
                {
                    targetSources.m_staticSources.push_back(autogenOutput);
                }
            }

            return TestImpact::BuildTargetDescriptor({ AZStd::move(buildMetaData), AZStd::move(targetSources) });
        }

        AZStd::vector<TestImpact::ProductionTargetDescriptor> CreateProductionTargetDescriptors()
        {
            AZStd::vector<TestImpact::ProductionTargetDescriptor> productionTargetDescriptors;
            productionTargetDescriptors.push_back(CreateBuildTargetDescriptor("Lib A", { "LibA_1.cpp", "LibA_2.cpp", "ProdAndTest.cpp" }));
            productionTargetDescriptors.push_back(CreateBuildTargetDescriptor(
                "Lib B", { "LibB_1.cpp" }, { {"LibB_AutogenInput.xml", {"LibB_2.cpp", "LibB_3.cpp"}} }));
            productionTargetDescriptors.push_back(CreateBuildTargetDescriptor(
                "Lib C", { "LibC_1.cpp", "LibC_2.cpp", "LibC_3.cpp" }));
            productionTargetDescriptors.push_back(CreateBuildTargetDescriptor("Lib Misc", { "LibMisc_1.cpp", "LibMisc_2.cpp" }));
            productionTargetDescriptors.push_back(CreateBuildTargetDescriptor("Lib Core", { "LibCore_1.cpp", "LibCore_2.cpp" }));
            productionTargetDescriptors.push_back(CreateBuildTargetDescriptor(
                "Lib Aux", { "LibAux_1.cpp", "LibAux_2.cpp", "LibAux_3.cpp" }));

            return productionTargetDescriptors;
        }

        AZStd::vector<TestImpact::TestTargetDescriptor> CreateTestTargetDescriptors()
        {
            AZStd::vector<TestImpact::TestTargetDescriptor> testTargetDescriptors;
            testTargetDescriptors.push_back(TestImpact::TestTargetDescriptor(CreateBuildTargetDescriptor("Test A", { "TestA.cpp" }), {}));
            testTargetDescriptors.push_back(TestImpact::TestTargetDescriptor(CreateBuildTargetDescriptor("Test B", { "TestB.cpp" }), {}));
            testTargetDescriptors.push_back(TestImpact::TestTargetDescriptor(CreateBuildTargetDescriptor("Test C", { "TestC.cpp" }), {}));
            testTargetDescriptors.push_back(TestImpact::TestTargetDescriptor(CreateBuildTargetDescriptor("Test Misc", { "TestMisc.cpp", "ProdAndTest.cpp" }), {}));
            testTargetDescriptors.push_back(TestImpact::TestTargetDescriptor(CreateBuildTargetDescriptor("Test Core", { "TestCore.cpp" }), {}));
            testTargetDescriptors.push_back(TestImpact::TestTargetDescriptor(CreateBuildTargetDescriptor("Test Aux", { "TestAux.cpp" }), {}));

            return testTargetDescriptors;
        }

        AZStd::vector<TestImpact::ProductionTargetDescriptor> CreateProductionTargetDescriptorsWithSharedSources()
        {
            auto productionTargetDescriptors = CreateProductionTargetDescriptors();
            productionTargetDescriptors.push_back(CreateBuildTargetDescriptor("Lib Shared", { "LibShared.cpp", "LibAux_2.cpp", "LibB_2.cpp" }));

            return productionTargetDescriptors;
        }

        AZStd::vector<TestImpact::TestTargetDescriptor> CreateTestTargetDescriptorsWithSharedSources()
        {
            auto testTargetDescriptors = CreateTestTargetDescriptors();
            testTargetDescriptors.push_back(TestImpact::TestTargetDescriptor(CreateBuildTargetDescriptor("Test Shared", { "TestShared.cpp" }), {}));

            return testTargetDescriptors;
        }

        AZStd::vector<TestImpact::SourceCoveringTests> CreateSourceCoveringTestList()
        {
            AZStd::vector<TestImpact::SourceCoveringTests> sourceCoveringTestList;
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibA_1.cpp"), AZStd::vector<AZStd::string>{"Test A"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibA_2.cpp"), AZStd::vector<AZStd::string>{"Test A", "Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibB_1.cpp"), AZStd::vector<AZStd::string>{"Test B", "Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibB_2.cpp"), AZStd::vector<AZStd::string>{"Test B"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibB_3.cpp"), AZStd::vector<AZStd::string>{"Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibC_1.cpp"), AZStd::vector<AZStd::string>{"Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibC_2.cpp"), AZStd::vector<AZStd::string>{"Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibC_3.cpp"), AZStd::vector<AZStd::string>{"Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibMisc_1.cpp"), AZStd::vector<AZStd::string>{"Test Misc"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibMisc_2.cpp"), AZStd::vector<AZStd::string>{"Test Misc"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibCore_1.cpp"), AZStd::vector<AZStd::string>{"Test Core", "Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibCore_2.cpp"), AZStd::vector<AZStd::string>{"Test Core", "Test A", "Test B", "Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibAux_1.cpp"), AZStd::vector<AZStd::string>{"Test Aux", "Test B", "Test Misc"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibAux_2.cpp"), AZStd::vector<AZStd::string>{"Test Aux", "Test C", "Test Misc"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibAux_3.cpp"), AZStd::vector<AZStd::string>{"Test Aux", "Test B", "Test C", "Test Misc"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("TestA.cpp"), AZStd::vector<AZStd::string>{"Test A"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("TestB.cpp"), AZStd::vector<AZStd::string>{"Test B"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("TestC.cpp"), AZStd::vector<AZStd::string>{"Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("TestCore.cpp"), AZStd::vector<AZStd::string>{"Test Core"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("TestMisc.cpp"), AZStd::vector<AZStd::string>{"Test Misc"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("TestAux.cpp"), AZStd::vector<AZStd::string>{"Test Aux"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("ProdAndTest.cpp"), AZStd::vector<AZStd::string>{"Test A"}));

            return sourceCoveringTestList;
        }

        AZStd::vector<TestImpact::SourceCoveringTests> CreateSourceCoveringTestListWithSharedSources()
        {
            AZStd::vector<TestImpact::SourceCoveringTests> sourceCoveringTestList;
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibA_1.cpp"), AZStd::vector<AZStd::string>{"Test A"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibA_2.cpp"), AZStd::vector<AZStd::string>{"Test A", "Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibB_1.cpp"), AZStd::vector<AZStd::string>{"Test B", "Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibB_2.cpp"), AZStd::vector<AZStd::string>{"Test B"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibB_3.cpp"), AZStd::vector<AZStd::string>{"Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibC_1.cpp"), AZStd::vector<AZStd::string>{"Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibC_2.cpp"), AZStd::vector<AZStd::string>{"Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibC_3.cpp"), AZStd::vector<AZStd::string>{"Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibMisc_1.cpp"), AZStd::vector<AZStd::string>{"Test Misc"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibMisc_2.cpp"), AZStd::vector<AZStd::string>{"Test Misc"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibCore_1.cpp"), AZStd::vector<AZStd::string>{"Test Core", "Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibCore_2.cpp"), AZStd::vector<AZStd::string>{"Test Core", "Test A", "Test B", "Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibAux_1.cpp"), AZStd::vector<AZStd::string>{"Test Aux", "Test B", "Test Misc"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibAux_2.cpp"), AZStd::vector<AZStd::string>{"Test Aux", "Test C", "Test Misc", "Test Shared"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibAux_3.cpp"), AZStd::vector<AZStd::string>{"Test Aux", "Test B", "Test C", "Test Misc"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("TestA.cpp"), AZStd::vector<AZStd::string>{"Test A"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("TestB.cpp"), AZStd::vector<AZStd::string>{"Test B"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("TestC.cpp"), AZStd::vector<AZStd::string>{"Test C"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("TestCore.cpp"), AZStd::vector<AZStd::string>{"Test Core"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("TestMisc.cpp"), AZStd::vector<AZStd::string>{"Test Misc"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("TestAux.cpp"), AZStd::vector<AZStd::string>{"Test Aux"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("ProdAndTest.cpp"), AZStd::vector<AZStd::string>{"Test A"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("LibShared.cpp"), AZStd::vector<AZStd::string>{"Test Aux", "Test Misc", "Test B", "Test C", "Test Shared"}));
            sourceCoveringTestList.push_back(TestImpact::SourceCoveringTests(TestImpact::RepoPath("TestShared.cpp"), AZStd::vector<AZStd::string>{"Test Shared"}));

            return sourceCoveringTestList;
        }

        AZStd::vector<TestImpact::SourceCoveringTests> CreateSourceCoverageTestsWithoutSpecifiedSource(
            AZStd::vector<TestImpact::SourceCoveringTests> sourceCoveringTestsList, const TestImpact::RepoPath& sourceToRemove)
        {
            AZStd::erase_if(sourceCoveringTestsList, [&sourceToRemove](const TestImpact::SourceCoveringTests& sourceCoveringTests)
            {
                if (sourceToRemove == "LibB_AutogenInput.xml")
                {
                    return sourceCoveringTests.GetPath().c_str() == sourceToRemove ||
                        sourceCoveringTests.GetPath() == "LibB_2.cpp" ||
                        sourceCoveringTests.GetPath() == "LibB_3.cpp";
                }
                else
                {
                    return sourceCoveringTests.GetPath().c_str() == sourceToRemove;
                }
            });

            return sourceCoveringTestsList;
        }

        const SourceMap ProductionSources =
        {
            {
                "LibA_1.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test A", "Test C"},

                    // m_updateParentYesCoverageNo
                    {},

                    // m_updateParentYesCoverageYes
                    {"Test A"},

                    // m_updateParentNoCoverageYes
                    {"Test A"},

                    // m_updateParentNoCoverageYes
                    {"Test A"}
                }
            },
            {
                "LibA_2.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test A"},

                    // m_updateParentYesCoverageNo
                    {},

                    // m_updateParentYesCoverageYes
                    {"Test A", "Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test A", "Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test A", "Test C"}
                }
            },
            {
                "LibB_1.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test B", "Test C"},

                    // m_updateParentYesCoverageNo
                    {},

                    // m_updateParentYesCoverageYes
                    {"Test B", "Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test B", "Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test B", "Test C"}
                }
            },
            {
                "LibB_2.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test Aux", "Test B", "Test C", "Test Misc", "Test Shared"},

                    // m_updateParentYesCoverageNo
                    {},

                    // m_updateParentYesCoverageYes
                    {"Test B"},

                    // m_updateParentNoCoverageYes
                    {"Test B"},

                    // m_updateParentNoCoverageYes
                    {"Test B"}
                }
            },
            {
                "LibB_3.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test B", "Test C"},

                    // m_updateParentYesCoverageNo
                    {},

                    // m_updateParentYesCoverageYes
                    {"Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test C"}
                }
            },
            {
                "LibC_1.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test C"},

                    // m_updateParentYesCoverageNo
                    {},

                    // m_updateParentYesCoverageYes
                    {"Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test C"}
                }
            },
            {
                "LibC_2.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test C"},

                    // m_updateParentYesCoverageNo
                    {},

                    // m_updateParentYesCoverageYes
                    {"Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test C"}
                }
            },
            {
                "LibC_3.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test C"},

                    // m_updateParentYesCoverageNo
                    {},

                    // m_updateParentYesCoverageYes
                    {"Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test C"}
                }
            },
            {
                "LibMisc_1.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test Misc"},

                    // m_updateParentYesCoverageNo
                    {},

                    // m_updateParentYesCoverageYes
                    {"Test Misc"},

                    // m_updateParentNoCoverageYes
                    {"Test Misc"},

                    // m_updateParentNoCoverageYes
                    {"Test Misc"}
                }
            },
            {
                "LibMisc_2.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test Misc"},

                    // m_updateParentYesCoverageNo
                    {},

                    // m_updateParentYesCoverageYes
                    {"Test Misc"},

                    // m_updateParentNoCoverageYes
                    {"Test Misc"},

                    // m_updateParentNoCoverageYes
                    {"Test Misc"}
                }
            },
            {
                "LibCore_1.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test Core", "Test A", "Test B", "Test C"},

                    // m_updateParentYesCoverageNo
                    {},

                    // m_updateParentYesCoverageYes
                    {"Test Core", "Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test Core", "Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test Core", "Test C"}
                }
            },
            {
                "LibCore_2.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test Core", "Test C"},

                    // m_updateParentYesCoverageNo
                    {},

                    // m_updateParentYesCoverageYes
                    {"Test Core", "Test A", "Test B", "Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test Core", "Test A", "Test B", "Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test Core", "Test A", "Test B", "Test C"}
                }
            },
            {
                "LibAux_1.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test Aux", "Test B", "Test C", "Test Misc", "Test Shared"},

                    // m_updateParentYesCoverageNo
                    {},

                    // m_updateParentYesCoverageYes
                    {"Test Aux", "Test B", "Test Misc"},

                    // m_updateParentNoCoverageYes
                    {"Test Aux", "Test B", "Test Misc"},

                    // m_updateParentNoCoverageYes
                    {"Test Aux", "Test B", "Test Misc"}
                }
            },
            {
                "LibAux_2.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test Aux", "Test B", "Test C", "Test Misc", "Test Shared"},

                    // m_updateParentYesCoverageNo
                    {},

                    // m_updateParentYesCoverageYes
                    {"Test Aux", "Test C", "Test Misc", "Test Shared"},

                    // m_updateParentNoCoverageYes
                    {"Test Aux", "Test C", "Test Misc", "Test Shared"},

                    // m_updateParentNoCoverageYes
                    {"Test Aux", "Test C", "Test Misc", "Test Shared"}
                }
            },
            {
                "LibAux_3.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test Aux", "Test B", "Test C", "Test Misc", "Test Shared"},

                    // m_updateParentYesCoverageNo
                    {},

                    // m_updateParentYesCoverageYes
                    {"Test Aux", "Test B", "Test C", "Test Misc"},

                    // m_updateParentNoCoverageYes
                    {"Test Aux", "Test B", "Test C", "Test Misc"},

                    // m_updateParentNoCoverageYes
                    {"Test Aux", "Test B", "Test C", "Test Misc"}
                }
            },
            {
                "LibShared.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test Aux", "Test B", "Test C", "Test Misc", "Test Shared"},

                    // m_updateParentYesCoverageNo
                    {},

                    // m_updateParentYesCoverageYes
                    {"Test Aux", "Test B", "Test C", "Test Misc", "Test Shared"},

                    // m_updateParentNoCoverageYes
                    {"Test Aux", "Test Misc", "Test B", "Test C", "Test Shared"},

                    // m_updateParentNoCoverageYes
                    {"Test Aux", "Test Misc", "Test B", "Test C", "Test Shared"}
                }
            }
        };

        const SourceMap AutogenInputSources =
        {
            {
                "LibB_AutogenInput.xml",
                {
                    // m_createParentYesCoverageNo
                    {"Test Aux", "Test B", "Test C", "Test Misc", "Test Shared"},

                    // m_updateParentYesCoverageNo
                    {},

                    // m_updateParentYesCoverageYes
                    {"Test B", "Test C"},

                    // m_updateParentNoCoverageYes
                    {},

                    // m_updateParentNoCoverageYes
                    {}
                }
            }
        };

        const SourceMap  TestSources =
        {
            {
                "TestA.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test A"},

                    // m_updateParentYesCoverageNo
                    {"Test A"},

                    // m_updateParentYesCoverageYes
                    {"Test A"},

                    // m_updateParentNoCoverageYes
                    {"Test A"},

                    // m_updateParentNoCoverageYes
                    {"Test A"}
                }
            },
            {
                "TestB.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test B"},

                    // m_updateParentYesCoverageNo
                    {"Test B"},

                    // m_updateParentYesCoverageYes
                    {"Test B"},

                    // m_updateParentNoCoverageYes
                    {"Test B"},

                    // m_updateParentNoCoverageYes
                    {"Test B"}
                }
            },
            {
                "TestC.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test C"},

                    // m_updateParentYesCoverageNo
                    {"Test C"},

                    // m_updateParentYesCoverageYes
                    {"Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test C"},

                    // m_updateParentNoCoverageYes
                    {"Test C"}
                }
            },
            {
                "TestMisc.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test Misc"},

                    // m_updateParentYesCoverageNo
                    {"Test Misc"},

                    // m_updateParentYesCoverageYes
                    {"Test Misc"},

                    // m_updateParentNoCoverageYes
                    {"Test Misc"},

                    // m_updateParentNoCoverageYes
                    {"Test Misc"}
                }
            },
            {
                "TestCore.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test Core"},

                    // m_updateParentYesCoverageNo
                    {"Test Core"},

                    // m_updateParentYesCoverageYes
                    {"Test Core"},

                    // m_updateParentNoCoverageYes
                    {"Test Core"},

                    // m_updateParentNoCoverageYes
                    {"Test Core"}
                }
            },
            {
                "TestAux.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test Aux"},

                    // m_updateParentYesCoverageNo
                    {"Test Aux"},

                    // m_updateParentYesCoverageYes
                    {"Test Aux"},

                    // m_updateParentNoCoverageYes
                    {"Test Aux"},

                    // m_updateParentNoCoverageYes
                    {"Test Aux"}
                }
            },
            {
                "TestShared.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test Shared"},

                    // m_updateParentYesCoverageNo
                    {"Test Shared"},

                    // m_updateParentYesCoverageYes
                    {"Test Shared"},

                    // m_updateParentNoCoverageYes
                    {"Test Shared"},

                    // m_updateParentNoCoverageYes
                    {"Test Shared"}
                }
            }
        };

        const SourceMap  MixedTargetSources =
        {
            {
                "ProdAndTest.cpp",
                {
                    // m_createParentYesCoverageNo
                    {"Test A", "Test C", "Test Misc"},

                    // m_updateParentYesCoverageNo
                    {"Test Misc"},

                    // m_updateParentYesCoverageYes
                    {"Test Misc", "Test A"},

                    // m_updateParentNoCoverageYes
                    {"Test A"},

                    // m_updateParentNoCoverageYes
                    {"Test A"}
                }
            }
        };

        SourceMap GenerateSourceMap(size_t sourcesToInclude)
        {
            SourceMap sources;

            if (sourcesToInclude & Sources::Production)
            {
                sources.insert(ProductionSources.begin(), ProductionSources.end());
            }

            if (sourcesToInclude & Sources::AutogenInput)
            {
                sources.insert(AutogenInputSources.begin(), AutogenInputSources.end());
            }

            if (sourcesToInclude & Sources::Test)
            {
                sources.insert(TestSources.begin(), TestSources.end());
            }

            if (sourcesToInclude & Sources::Mixed)
            {
                sources.insert(MixedTargetSources.begin(), MixedTargetSources.end());
            }

            return sources;
        }
    }
}
