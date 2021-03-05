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

#include "EditorDefs.h"
#include <AzTest/AzTest.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <EditorEnvironment.h>

#include <QApplication>

class EditorLibTestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    virtual ~EditorLibTestEnvironment() {}

protected:
    void SetupEnvironment() override
    {
        AZ::Environment::Create(nullptr);
        AttachEditorAZEnvironment(AZ::Environment::GetInstance());
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
    }

    void TeardownEnvironment() override
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        DetachEditorAZEnvironment();
        AZ::Environment::Destroy();
    }
};

AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    // NOTE - this line makes this code different from AZ_UNIT_TEST_HOOK()
    QApplication app(argc, argv);
    // end
    AZ::Test::ApplyGlobalParameters(&argc, argv);
    AZ::Test::printUnusedParametersWarning(argc, argv);
    AZ::Test::addTestEnvironments({new EditorLibTestEnvironment});
    int result = RUN_ALL_TESTS();
    return result;
}

IMPLEMENT_TEST_EXECUTABLE_MAIN()
