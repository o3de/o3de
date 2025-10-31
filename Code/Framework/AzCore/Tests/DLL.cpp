/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/DLLTestVirtualClass.h>

using namespace AZ;

namespace UnitTest
{
#if !AZ_UNIT_TEST_SKIP_DLL_TEST

    class DLL
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            AZ::NameDictionary::Create();
        }

        void TearDown() override
        {
            AZ::NameDictionary::Destroy();

            LeakDetectionFixture::TearDown();
        }

        void LoadModule()
        {
            m_handle = DynamicModuleHandle::Create("AzCoreTestDLL");
            bool isLoaded = m_handle->Load(AZ::DynamicModuleHandle::LoadFlags::InitFuncRequired);
            ASSERT_TRUE(isLoaded) << "Could not load required test module: " << m_handle->GetFilename(); // failed to load the DLL, please check the output paths

            auto createModule = m_handle->GetFunction<CreateModuleClassFunction>(CreateModuleClassFunctionName);
            // if this fails, we cannot continue as we will just nullptr exception
            ASSERT_NE(nullptr, createModule) << "Unable to find create module function in module: " << CreateModuleClassFunctionName;
            m_module = createModule();
            ASSERT_NE(nullptr, m_module);
        }

        void UnloadModule()
        {
            auto destroyModule = m_handle->GetFunction<DestroyModuleClassFunction>(DestroyModuleClassFunctionName);
            ASSERT_NE(nullptr, destroyModule) << "Could not find the destroy function in the module: " << DestroyModuleClassFunctionName;
            destroyModule(m_module);

            m_handle->Unload();
            m_handle.reset();
        }

        AZStd::unique_ptr<DynamicModuleHandle> m_handle;
        Module* m_module = nullptr;
    };

    class TransformHandler
        : public TransformNotificationBus::Handler
    {
    public:
        int m_numEBusCalls = 0;

        void OnParentChanged(EntityId oldParent, EntityId newParent) override
        {
            EXPECT_FALSE(oldParent.IsValid());
            void* systemAllocatorAddress = (void*)(u64)newParent;
            azfree(systemAllocatorAddress); // free memory allocated in a different module, this should be fine as we share environment/allocators
            ++m_numEBusCalls;
        }
    };

#if AZ_TRAIT_DISABLE_FAILED_DLL_TESTS
    TEST_F(DLL, DISABLED_CrossModuleBusHandler)
#else
    TEST_F(DLL, CrossModuleBusHandler)
#endif // AZ_TRAIT DISABLE_FAILED_DLL_TESTS
    {
        TransformHandler transformHandler;

        LoadModule();

        AZ::SerializeContext serializeContext;
        m_module->Reflect(&serializeContext);

        using DoTests = void(*)();
        DoTests runTests = m_handle->GetFunction<DoTests>("DoTests");
        EXPECT_NE(nullptr, runTests);

        // use the transform bus to verify that we can call EBus messages across modules
        transformHandler.BusConnect(EntityId());

        EXPECT_EQ(0, transformHandler.m_numEBusCalls);

        runTests();

        EXPECT_EQ(1, transformHandler.m_numEBusCalls);

        transformHandler.BusDisconnect(EntityId());

        UnloadModule();
    }

#if AZ_TRAIT_DISABLE_FAILED_DLL_TESTS
    TEST_F(DLL, DISABLED_CreateVariableFromModuleAndMain)
#else
    TEST_F(DLL, CreateVariableFromModuleAndMain)
#endif // AZ_TRAIT_DISABLE_FAILED_DLL_TESTS
    {
        LoadModule();

        const char* envVariableName = "My Variable";
        EnvironmentVariable<UnitTest::DLLTestVirtualClass> envVariable;

        // create owned environment variable (variable which uses vtable so it can't exist when the module is unloaded.
        typedef void(*CreateDLLVar)(const char*);
        CreateDLLVar createDLLVar = m_handle->GetFunction<CreateDLLVar>("CreateDLLTestVirtualClass");
        createDLLVar(envVariableName);

        envVariable = AZ::Environment::FindVariable<UnitTest::DLLTestVirtualClass>(envVariableName);
        EXPECT_TRUE(envVariable);
        EXPECT_EQ(1, envVariable->m_data);

        UnloadModule();

        // the variable is owned by the module (due to the vtable reference), once the module
        // is unloaded the variable should be unconstructed
        EXPECT_FALSE(envVariable);

        //////////////////////////////////////////////////////////////////////////
        // load the module and see if we recreate our variable
        LoadModule();

        // create variable
        createDLLVar = m_handle->GetFunction<CreateDLLVar>("CreateDLLTestVirtualClass");
        createDLLVar(envVariableName);

        envVariable = AZ::Environment::FindVariable<UnitTest::DLLTestVirtualClass>(envVariableName);
        EXPECT_TRUE(envVariable); // createDLLVar should construct the variable if already there
        EXPECT_EQ(1, envVariable->m_data);

        UnloadModule();

        //////////////////////////////////////////////////////////////////////////
        // Since the variable is valid till the last reference is gone, we have the option
        // to recreate the variable from a different module
        EXPECT_FALSE(envVariable); // Validate that the variable is not constructed

         // since the variable is destroyed, we can create it from a different module, the new module will be owner
        envVariable = AZ::Environment::CreateVariable<UnitTest::DLLTestVirtualClass>(envVariableName);
        EXPECT_TRUE(envVariable); // createDLLVar should construct the variable if already there
        EXPECT_EQ(1, envVariable->m_data);
    }

    TEST_F(DLL, CreateEnvironmentVariableThreadRace)
    {
        const int numThreads = 64;
        int values[numThreads] = { 0 };
        AZStd::thread threads[numThreads];
        AZ::EnvironmentVariable<int> envVar;
        for (int threadIdx = 0; threadIdx < numThreads; ++threadIdx)
        {
            auto threadFunc = [&values, &envVar, threadIdx]()
            {
                envVar = AZ::Environment::CreateVariable<int>("CreateEnvironmentVariableThreadRace", threadIdx);
                values[threadIdx] = *envVar;
            };

            threads[threadIdx] = AZStd::thread(threadFunc);
        }

        for (auto& thread : threads)
        {
            thread.join();
        }

        AZStd::unordered_set<int> uniqueValues;
        for (const int& value : values)
        {
            uniqueValues.insert(value);
        }

        EXPECT_EQ(1, uniqueValues.size());
    }

    TEST_F(DLL, LoadFailure)
    {
        auto handle = DynamicModuleHandle::Create("Not_a_DLL");
        bool isLoaded = handle->Load(AZ::DynamicModuleHandle::LoadFlags::InitFuncRequired);
        EXPECT_FALSE(isLoaded);

        bool isUnloaded = handle->Unload();
        EXPECT_FALSE(isUnloaded);
    }

    TEST_F(DLL, LoadModuleTwice)
    {
        auto handle = DynamicModuleHandle::Create("AzCoreTestDLL");
        bool isLoaded = handle->Load(AZ::DynamicModuleHandle::LoadFlags::InitFuncRequired);
        EXPECT_TRUE(isLoaded);
        EXPECT_TRUE(handle->IsLoaded());

        auto secondHandle = DynamicModuleHandle::Create("AzCoreTestDLL");
        isLoaded = secondHandle->Load(AZ::DynamicModuleHandle::LoadFlags::InitFuncRequired);
        EXPECT_TRUE(isLoaded);
        EXPECT_TRUE(handle->IsLoaded());
        EXPECT_TRUE(secondHandle->IsLoaded());

        bool isUnloaded = handle->Unload();
        EXPECT_TRUE(isUnloaded);
        EXPECT_FALSE(handle->IsLoaded());
        EXPECT_TRUE(secondHandle->IsLoaded());

        isUnloaded = secondHandle->Unload();
        EXPECT_TRUE(isUnloaded);
        EXPECT_FALSE(handle->IsLoaded());
        EXPECT_FALSE(secondHandle->IsLoaded());
    }

    TEST_F(DLL, NoLoadModule)
    {
        auto handle = DynamicModuleHandle::Create("AzCoreTestDLL");
        bool isLoaded = handle->Load(AZ::DynamicModuleHandle::LoadFlags::InitFuncRequired | AZ::DynamicModuleHandle::LoadFlags::NoLoad);
        EXPECT_FALSE(isLoaded);
        EXPECT_FALSE(handle->IsLoaded());
    }

    TEST_F(DLL, NoLoadLoadedModule)
    {
        auto handle = DynamicModuleHandle::Create("AzCoreTestDLL");
        bool isLoaded = handle->Load(AZ::DynamicModuleHandle::LoadFlags::InitFuncRequired);
        EXPECT_TRUE(isLoaded);
        EXPECT_TRUE(handle->IsLoaded());

        auto secondHandle = DynamicModuleHandle::Create("AzCoreTestDLL");
        isLoaded = secondHandle->Load(AZ::DynamicModuleHandle::LoadFlags::InitFuncRequired | AZ::DynamicModuleHandle::LoadFlags::NoLoad);
        EXPECT_TRUE(isLoaded);
        EXPECT_TRUE(handle->IsLoaded());
        EXPECT_TRUE(secondHandle->IsLoaded());

        bool isUnloaded = handle->Unload();
        EXPECT_TRUE(isUnloaded);
        EXPECT_FALSE(handle->IsLoaded());
        EXPECT_TRUE(secondHandle->IsLoaded());

        {
            // Check that the Module wasn't unloaded by the OS when unloading one DynamicModuleHandle (the OS should have 1 ref count left).
            isLoaded = handle->Load(AZ::DynamicModuleHandle::LoadFlags::InitFuncRequired | AZ::DynamicModuleHandle::LoadFlags::NoLoad);
            EXPECT_TRUE(isLoaded);
            EXPECT_TRUE(handle->IsLoaded());
            handle->Unload();
        }

        isUnloaded = secondHandle->Unload();
        EXPECT_TRUE(isUnloaded);
        EXPECT_FALSE(handle->IsLoaded());
        EXPECT_FALSE(secondHandle->IsLoaded());

        {
            // Check that the Module was unloaded by the OS.
            isLoaded = handle->Load(AZ::DynamicModuleHandle::LoadFlags::InitFuncRequired | AZ::DynamicModuleHandle::LoadFlags::NoLoad);
            EXPECT_FALSE(isLoaded);
            EXPECT_FALSE(handle->IsLoaded());
            handle->Unload();
        }
    }
#endif // !AZ_UNIT_TEST_SKIP_DLL_TEST
}
