/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    ~EditorLibTestEnvironment() override = default;

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
