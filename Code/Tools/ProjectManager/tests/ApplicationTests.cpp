/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Application.h>

namespace O3DE::ProjectManager
{
    class ProjectManagerApplicationTests 
        : public ::testing::Test
        , public ::UnitTest::AllocatorsBase
    {
    public:

        static void SetUpTestCase()
        {
            if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
            }

            s_application = AZStd::make_unique<ProjectManager::Application>(/*argc=*/nullptr, /*argv=*/nullptr);
        }

        static void TearDownTestCase()
        {
            s_application->TearDown();
            s_application.reset();

            if (AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            }
        }

        static AZStd::unique_ptr<ProjectManager::Application> s_application;
    };

    AZStd::unique_ptr<ProjectManager::Application> ProjectManagerApplicationTests::s_application;

    TEST_F(ProjectManagerApplicationTests, Application_Init_Succeeds)
    {
        EXPECT_TRUE(s_application->Init());
    }
}
