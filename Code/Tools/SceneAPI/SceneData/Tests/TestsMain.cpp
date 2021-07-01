/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Module/DynamicModuleHandle.h>
#include <SceneAPI/SceneData/SceneDataStandaloneAllocator.h>

class SceneDataTestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    virtual ~SceneDataTestEnvironment()
    {}

protected:
    void SetupEnvironment() override
    {
        AZ::Environment::Create(nullptr);
        AZ::SceneAPI::SceneDataStandaloneAllocator::Initialize(AZ::Environment::GetInstance());

        sceneCoreModule = AZ::DynamicModuleHandle::Create("SceneCore");
        AZ_Assert(sceneCoreModule, "SceneData unit tests failed to create SceneCore module.");
        [[maybe_unused]] bool loaded = sceneCoreModule->Load(false);
        AZ_Assert(loaded, "SceneData unit tests failed to load SceneCore module.");
        auto init = sceneCoreModule->GetFunction<AZ::InitializeDynamicModuleFunction>(AZ::InitializeDynamicModuleFunctionName);
        AZ_Assert(init, "SceneData unit tests failed to find the initialization function the SceneCore module.");
        (*init)(AZ::Environment::GetInstance());
    }

    void TeardownEnvironment() override
    {
        auto uninit = sceneCoreModule->GetFunction<AZ::UninitializeDynamicModuleFunction>(AZ::UninitializeDynamicModuleFunctionName);
        AZ_Assert(uninit, "SceneData unit tests failed to find the uninitialization function the SceneCore module.");
        (*uninit)();
        sceneCoreModule.reset();
        AZ::SceneAPI::SceneDataStandaloneAllocator::TearDown();
        AZ::Environment::Destroy();
    }

private:
    AZStd::unique_ptr<AZ::DynamicModuleHandle> sceneCoreModule;
};

AZ_UNIT_TEST_HOOK(new SceneDataTestEnvironment);
