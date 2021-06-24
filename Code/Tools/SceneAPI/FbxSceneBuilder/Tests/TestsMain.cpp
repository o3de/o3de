/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Memory/SystemAllocator.h>

class FbxSceneBuilderTestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    virtual ~FbxSceneBuilderTestEnvironment()
    {}

protected:
    void SetupEnvironment() override
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

        sceneCoreModule = AZ::DynamicModuleHandle::Create("SceneCore");
        AZ_Assert(sceneCoreModule, "FbxSceneBuilder unit tests failed to create SceneCore module.");
        bool loaded = sceneCoreModule->Load(false);
        AZ_Assert(loaded, "FbxSceneBuilder unit tests failed to load SceneCore module.");
        auto init = sceneCoreModule->GetFunction<AZ::InitializeDynamicModuleFunction>(AZ::InitializeDynamicModuleFunctionName);
        AZ_Assert(init, "FbxSceneBuilder unit tests failed to find the initialization function the SceneCore module.");
        (*init)(AZ::Environment::GetInstance());

        sceneDataModule = AZ::DynamicModuleHandle::Create("SceneData");
        AZ_Assert(sceneDataModule, "SceneData unit tests failed to create SceneData module.");
        loaded = sceneDataModule->Load(false);
        AZ_Assert(loaded, "FbxSceneBuilder unit tests failed to load SceneData module.");
        init = sceneDataModule->GetFunction<AZ::InitializeDynamicModuleFunction>(AZ::InitializeDynamicModuleFunctionName);
        AZ_Assert(init, "FbxSceneBuilder unit tests failed to find the initialization function the SceneData module.");
        (*init)(AZ::Environment::GetInstance());
    }

    void TeardownEnvironment() override
    {
        auto uninit = sceneDataModule->GetFunction<AZ::UninitializeDynamicModuleFunction>(AZ::UninitializeDynamicModuleFunctionName);
        AZ_Assert(uninit, "FbxSceneBuilder unit tests failed to find the uninitialization function the SceneData module.");
        (*uninit)();
        sceneDataModule.reset();
        
        uninit = sceneCoreModule->GetFunction<AZ::UninitializeDynamicModuleFunction>(AZ::UninitializeDynamicModuleFunctionName);
        AZ_Assert(uninit, "FbxSceneBuilder unit tests failed to find the uninitialization function the SceneCore module.");
        (*uninit)();
        sceneCoreModule.reset();

        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }

private:
    AZStd::unique_ptr<AZ::DynamicModuleHandle> sceneCoreModule;
    AZStd::unique_ptr<AZ::DynamicModuleHandle> sceneDataModule;
};

AZ_UNIT_TEST_HOOK(new FbxSceneBuilderTestEnvironment);

