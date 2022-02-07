/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/Utils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Cli/PythonBindings.h>
#include <Cli/PythonCliBindings.h>
#include <ProjectManager_Test_Traits_Platform.h>

#include <QDir>

namespace O3DE::ProjectManager
{
    class PythonBindingsTests 
        : public ::UnitTest::ScopedAllocatorSetupFixture
    {
    public:

        PythonBindingsTests()
        {
            const AZStd::string engineRootPath{ AZ::Test::GetEngineRootPath() };
            m_o3deCli = AZStd::make_unique<PythonBindings>(new PythonCliBindings(AZ::IO::PathView(engineRootPath)));
        }

        ~PythonBindingsTests()
        {
            m_o3deCli.reset();
        }

        AZStd::unique_ptr<ProjectManager::PythonBindings> m_o3deCli;
    };

    TEST_F(PythonBindingsTests, PythonBindings_Start_Python_Succeeds)
    {
        EXPECT_TRUE(m_o3deCli->PythonStarted());
    }

    TEST_F(PythonBindingsTests, PythonBindings_Create_Project_Succeeds)
    {
        ASSERT_TRUE(m_o3deCli->PythonStarted());

        auto templateResults = m_o3deCli->GetProjectTemplates();
        ASSERT_TRUE(templateResults.IsSuccess());

        QVector<ProjectTemplateInfo> templates = templateResults.GetValue();
        ASSERT_FALSE(templates.isEmpty());

        // use the first registered template
        QString templatePath = templates.at(0).m_path;

        AZ::Test::ScopedAutoTempDirectory tempDir;

        ProjectInfo projectInfo;
        projectInfo.m_path = QDir::toNativeSeparators(QString(tempDir.GetDirectory()) + "/" + "TestProject");
        projectInfo.m_projectName = "TestProjectName";

        auto result = m_o3deCli->CreateProject(templatePath, projectInfo);
        EXPECT_TRUE(result.IsSuccess());

        ProjectInfo resultProjectInfo = result.GetValue();
        EXPECT_EQ(projectInfo.m_path, resultProjectInfo.m_path);
        EXPECT_EQ(projectInfo.m_projectName, resultProjectInfo.m_projectName);
    }
}
