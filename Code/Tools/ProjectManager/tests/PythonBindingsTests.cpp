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
#include <PythonBindings.h>
#include <ProjectManager_Test_Traits_Platform.h>

#include <QDir>

namespace O3DE::ProjectManager
{
    class TestablePythonBindings
        : public PythonBindings
    {
    public:
        TestablePythonBindings(const AZ::IO::PathView& enginePath)
            : PythonBindings(enginePath)
        {
        }

        void TestOnStdOut(const char* msg)
        {
            PythonBindings::OnStdOut(msg);
        }

        void TestOnStdError(const char* msg)
        {
            PythonBindings::OnStdError(msg);
        }

        //! override with an implementation that won't do anything
        //! so we avoid modifying the manifest 
        bool RemoveInvalidProjects() override
        {
            constexpr bool removalResult = true;
            return removalResult;
        }
    };

    class PythonBindingsTests 
        : public ::UnitTest::LeakDetectionFixture
        , public AZ::Debug::TraceMessageBus::Handler
    {
    public:

        void SetUp() override
        {
            const AZStd::string engineRootPath{ AZ::Test::GetEngineRootPath() };
            m_pythonBindings = AZStd::make_unique<TestablePythonBindings>(AZ::IO::PathView(engineRootPath));
        }

        void TearDown() override
        {
            m_pythonBindings.reset();
        }

        //! AZ::Debug::TraceMessageBus
        bool OnOutput(const char*, const char* message) override
        {
            m_gatheredMessages.emplace_back(message);
            return true;
        }

        AZStd::unique_ptr<ProjectManager::TestablePythonBindings> m_pythonBindings;
        AZStd::vector<AZStd::string> m_gatheredMessages;
    };

    TEST_F(PythonBindingsTests, PythonBindings_Start_Python_Succeeds)
    {
        EXPECT_TRUE(m_pythonBindings->PythonStarted());
    }

    TEST_F(PythonBindingsTests, PythonBindings_Create_Project_Succeeds)
    {
        ASSERT_TRUE(m_pythonBindings->PythonStarted());

        auto templateResults = m_pythonBindings->GetProjectTemplates();
        ASSERT_TRUE(templateResults.IsSuccess());

        QVector<ProjectTemplateInfo> templates = templateResults.GetValue();
        ASSERT_FALSE(templates.isEmpty());

        // use the first registered template
        QString templatePath = templates.at(0).m_path;

        AZ::Test::ScopedAutoTempDirectory tempDir;

        ProjectInfo projectInfo;
        projectInfo.m_path = QDir::toNativeSeparators(QString(tempDir.GetDirectory()) + "/" + "TestProject");
        projectInfo.m_projectName = "TestProjectName";

        constexpr bool registerProject = false;
        auto result = m_pythonBindings->CreateProject(templatePath, projectInfo, registerProject);
        EXPECT_TRUE(result.IsSuccess());

        ProjectInfo resultProjectInfo = result.GetValue();
        EXPECT_EQ(projectInfo.m_path, resultProjectInfo.m_path);
        EXPECT_EQ(projectInfo.m_projectName, resultProjectInfo.m_projectName);
    }

    TEST_F(PythonBindingsTests, PythonBindings_Print_Percent_Does_Not_Crash)
    {
        bool testMessageFound = false;
        bool testErrorFound = false;
        const char* testMessage = "PythonTestMessage%";
        const char* testError = "ERROR:root:PythonTestError%";

        AZ::Debug::TraceMessageBus::Handler::BusConnect();

        m_pythonBindings->TestOnStdOut(testMessage);
        m_pythonBindings->TestOnStdError(testError);

        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();

        for (const auto& message : m_gatheredMessages)
        {
            if (message.contains(testMessage))
            {
                testMessageFound = true;
            }
            else if (message.contains(testError))
            {
                testErrorFound = true;
            }
        }

        EXPECT_TRUE(testMessageFound);
        EXPECT_TRUE(testErrorFound);
    }
} // namespace O3DE::ProjectManager
