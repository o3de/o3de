/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysX_precompiled.h>
#include <PhysXTestEnvironment.h>
#include <PhysX_Traits_Platform.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/UnitTest/UnitTest.h>

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/Utils.h>
#include <ComponentDescriptors.h>

#include <SystemComponent.h>

#include <BoxColliderComponent.h>
#include <CapsuleColliderComponent.h>
#include <SphereColliderComponent.h>

#include <Configuration/PhysXSettingsRegistryManager.h>
#include <System/PhysXCookingParams.h>
#include <PhysX/Debug/PhysXDebugInterface.h>

#include <Tests/PhysXTestCommon.h>

namespace PhysX
{
    /*static*/ bool Environment::s_enablePvd = false;

    PhysXApplication::PhysXApplication()
        : m_physXSystem(new TestUtils::Test_PhysXSettingsRegistryManager(), PxCooking::GetRealTimeCookingParams())
    {

    }

    AZ::ComponentTypeList PhysXApplication::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components = AZ::ComponentApplication::GetRequiredSystemComponents();
        components.insert(components.end(),
            {
                azrtti_typeid<AZ::MemoryComponent>(),
                azrtti_typeid<AZ::AssetManagerComponent>(),
                azrtti_typeid<AZ::JobManagerComponent>(),
                azrtti_typeid<AZ::StreamerComponent>(),
                azrtti_typeid<PhysX::SystemComponent>()
            });

        return components;
    }

    void PhysXApplication::CreateReflectionManager()
    {
        AZ::ComponentApplication::CreateReflectionManager();

        for (AZ::ComponentDescriptor* descriptor : GetDescriptors())
        {
            RegisterComponentDescriptor(descriptor);
        }
    }

    void PhysXApplication::Destroy()
    {
        AZ::ComponentApplication::Destroy();
        m_physXSystem.Shutdown();
    }

    void Environment::SetupInternal()
    {
#if AZ_TRAIT_UNITTEST_USE_TEST_RUNNER_ENVIRONMENT
        AZ::EnvironmentInstance inst = AZ::Test::GetPlatform().GetTestRunnerEnvironment();
        AZ::Environment::Attach(inst);
#endif
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

        m_fileIo = AZStd::make_unique<AZ::IO::LocalFileIO>();

        AZ::IO::FileIOBase::SetInstance(m_fileIo.get());

        char testDir[AZ_MAX_PATH_LEN];
        m_fileIo->ConvertToAbsolutePath("Test.Assets/Gems/PhysX/Code/Tests", testDir, AZ_MAX_PATH_LEN);
        m_fileIo->SetAlias("@test@", testDir);

        LoadPhysXLibraryModules();

        // Create application and descriptor
        m_application = aznew PhysXApplication;
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_useExistingAllocator = true;

        // Set up gems other than PhysX for loading
        AZ::DynamicModuleDescriptor dynamicModuleDescriptor;
        dynamicModuleDescriptor.m_dynamicLibraryPath = "LmbrCentral";
        appDesc.m_modules.push_back(dynamicModuleDescriptor);

        // Create system entity
        AZ::ComponentApplication::StartupParameters startupParams;
        m_systemEntity = m_application->Create(appDesc, startupParams);
        AZ_TEST_ASSERT(m_systemEntity);
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (serializeContext)
        {
            // The reflection for generic physics API types which PhysX relies on happens in AzFramework and is not
            // called by PhysX itself, so we have to make sure it is called here
            Physics::ReflectionUtils::ReflectPhysicsApi(serializeContext);
            m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(serializeContext);
        }

        m_systemEntity->Init();
        m_systemEntity->Activate();

        if (s_enablePvd)
        {
            auto* debug = AZ::Interface<PhysX::Debug::PhysXDebugInterface>::Get();
            if (debug)
            {
                debug->ConnectToPvd();
            }
        }
    }

    void Environment::TeardownInternal()
    {

        if (s_enablePvd)
        {
            auto* debug = AZ::Interface<PhysX::Debug::PhysXDebugInterface>::Get();
            if (debug)
            {
                debug->DisconnectFromPvd();
            }
        }

        AZ::IO::FileIOBase::SetInstance(nullptr);


        m_transformComponentDescriptor.reset();
        m_fileIo.reset();
        m_application->Destroy();
        delete m_application;

        UnloadPhysXLibraryModules();

    }


    void Environment::LoadPhysXLibraryModules()
    {
        
#if AZ_TRAIT_PHYSX_FORCE_LOAD_MODULES
        m_physXLibraryModules = AZStd::make_unique<PhysXLibraryModules>();

        // Load PhysX SDK dynamic libraries when running unit tests, otherwise the symbol import will fail in InitializePhysXSdk
        // Normally this is done in the PhysX Gem's module code, but is not currently done in unit tests.
        {
            const AZStd::vector<AZ::OSString> physXModuleNames = { "PhysX", "PhysXCooking", "PhysXFoundation", "PhysXCommon" };
            for (const auto& physXModuleName : physXModuleNames)
            {
                AZ::OSString modulePathName = physXModuleName;
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::ResolveModulePath, modulePathName);

                AZStd::unique_ptr<AZ::DynamicModuleHandle> physXModule = AZ::DynamicModuleHandle::Create(modulePathName.c_str());
                bool ok = physXModule->Load(false/*isInitializeFunctionRequired*/);
                AZ_Error("PhysX::Module", ok, "Error loading %s module", physXModuleName.c_str());

                m_physXLibraryModules->push_back(AZStd::move(physXModule));
            }
        }
#endif
    }

    void Environment::UnloadPhysXLibraryModules()
    {
#if AZ_TRAIT_PHYSX_FORCE_LOAD_MODULES
        // Unload modules in reverse order that were loaded
        for (auto it = m_physXLibraryModules->rbegin(); it != m_physXLibraryModules->rend(); ++it)
        {
            it->reset();
        }
        m_physXLibraryModules.reset();
#endif
    }

} // namespace PhysX
