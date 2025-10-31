/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Application.h>
#include <ProjectManager_Test_Traits_Platform.h>
#include "MockPythonBindings.h"

namespace O3DE::ProjectManager
{
    class ProjectManagerApplicationTests 
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            m_application = AZStd::make_unique<ProjectManager::Application>();
        }

        void TearDown() override
        {
            m_application.reset();
        }

        AZStd::unique_ptr<ProjectManager::Application> m_application;
    };

#if AZ_TRAIT_DISABLE_FAILED_PROJECT_MANAGER_TESTS
    TEST_F(ProjectManagerApplicationTests, DISABLED_Application_Init_Succeeds)
#else
    TEST_F(ProjectManagerApplicationTests, Application_Init_Succeeds)
#endif // !AZ_TRAIT_DISABLE_FAILED_PROJECT_MANAGER_TESTS
    {
        using ::testing::NiceMock;
        using ::testing::Return;

        // mock python bindings because those have separate tests and we want
        // to avoid modifying the manifest that other tests may be trying to read
        auto pythonBindings = AZStd::make_unique<NiceMock<MockPythonBindings>>();

        EngineInfo engineInfo;
        engineInfo.m_registered = true;
        ON_CALL(*pythonBindings, GetEngineInfo()).WillByDefault(Return(AZ::Success(AZStd::move(engineInfo))));
        ON_CALL(*pythonBindings, PythonStarted()).WillByDefault(Return(true));
        ON_CALL(*pythonBindings, StopPython()).WillByDefault(Return(true));

        // gem repos currently pop up a messagebox if no gem repos are found
        // so we must return an empty list
        ON_CALL(*pythonBindings, GetAllGemRepoInfos()).WillByDefault(Return(AZ::Success(QVector<GemRepoInfo>())));

        // we don't want to interact with actual GUI or display it
        EXPECT_TRUE(m_application->Init(/*interactive=*/false, AZStd::move(pythonBindings)));
    }
}
