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

namespace O3DE::ProjectManager
{
    class ProjectManagerApplicationTests 
        : public ::UnitTest::ScopedAllocatorSetupFixture
    {
    public:

        ProjectManagerApplicationTests()
        {
            m_application = AZStd::make_unique<ProjectManager::Application>();
        }

        ~ProjectManagerApplicationTests()
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
        // we don't want to interact with actual GUI or display it
        EXPECT_TRUE(m_application->Init(/*interactive=*/false));
    }
}
