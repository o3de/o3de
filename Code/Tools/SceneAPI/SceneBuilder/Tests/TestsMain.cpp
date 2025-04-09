/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Memory/SystemAllocator.h>

class SceneBuilderTestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    virtual ~SceneBuilderTestEnvironment()
    {}

protected:
    void SetupEnvironment() override
    {
        sceneCoreModule = AZ::DynamicModuleHandle::Create("SceneCore");
        AZ_Assert(sceneCoreModule, "SceneBuilder unit tests failed to create SceneCore module.");
        [[maybe_unused]] bool loaded = sceneCoreModule->Load();
        AZ_Assert(loaded, "SceneBuilder unit tests failed to load SceneCore module.");
        auto init = sceneCoreModule->GetFunction<AZ::InitializeDynamicModuleFunction>(AZ::InitializeDynamicModuleFunctionName);
        AZ_Assert(init, "SceneBuilder unit tests failed to find the initialization function the SceneCore module.");
        (*init)();

        sceneDataModule = AZ::DynamicModuleHandle::Create("SceneData");
        AZ_Assert(sceneDataModule, "SceneData unit tests failed to create SceneData module.");
        loaded = sceneDataModule->Load();
        AZ_Assert(loaded, "SceneBuilder unit tests failed to load SceneData module.");
        init = sceneDataModule->GetFunction<AZ::InitializeDynamicModuleFunction>(AZ::InitializeDynamicModuleFunctionName);
        AZ_Assert(init, "SceneBuilder unit tests failed to find the initialization function the SceneData module.");
        (*init)();
    }

    void TeardownEnvironment() override
    {
        auto uninit = sceneDataModule->GetFunction<AZ::UninitializeDynamicModuleFunction>(AZ::UninitializeDynamicModuleFunctionName);
        AZ_Assert(uninit, "SceneBuilder unit tests failed to find the uninitialization function the SceneData module.");
        (*uninit)();
        sceneDataModule.reset();
        
        uninit = sceneCoreModule->GetFunction<AZ::UninitializeDynamicModuleFunction>(AZ::UninitializeDynamicModuleFunctionName);
        AZ_Assert(uninit, "SceneBuilder unit tests failed to find the uninitialization function the SceneCore module.");
        (*uninit)();
        sceneCoreModule.reset();
    }

private:
    AZStd::unique_ptr<AZ::DynamicModuleHandle> sceneCoreModule;
    AZStd::unique_ptr<AZ::DynamicModuleHandle> sceneDataModule;
};

AZ_UNIT_TEST_HOOK(new SceneBuilderTestEnvironment);

