/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysXTestEnvironment.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/Physics/Utils.h>
#include <AzFramework/Physics/Material/PhysicsMaterialSystemComponent.h>
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
        : m_physXSystem(AZStd::make_unique<TestUtils::Test_PhysXSettingsRegistryManager>(), PxCooking::GetRealTimeCookingParams())
    {

    }

    AZ::ComponentTypeList PhysXApplication::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components = AZ::ComponentApplication::GetRequiredSystemComponents();
        components.insert(components.end(),
            {
                azrtti_typeid<AZ::AssetManagerComponent>(),
                azrtti_typeid<AZ::JobManagerComponent>(),
                azrtti_typeid<AZ::StreamerComponent>(),

                azrtti_typeid<AzFramework::AssetCatalogComponent>(),
                azrtti_typeid<Physics::MaterialSystemComponent>(),

                azrtti_typeid<PhysX::SystemComponent>()
            });

        return components;
    }

    void PhysXApplication::CreateReflectionManager()
    {
        AZ::ComponentApplication::CreateReflectionManager();

        const AZStd::list<AZ::ComponentDescriptor*> azFrameworkSystemDescriptors =
        {
            AzFramework::AssetCatalogComponent::CreateDescriptor(),
            Physics::MaterialSystemComponent::CreateDescriptor()
        };

        for (AZ::ComponentDescriptor* descriptor : azFrameworkSystemDescriptors)
        {
            RegisterComponentDescriptor(descriptor);
        }

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
        m_fileIo = AZStd::make_unique<AZ::IO::LocalFileIO>();

        AZ::IO::FileIOBase::SetInstance(m_fileIo.get());

        AZ::IO::FixedMaxPath testDir = AZ::Utils::GetExecutableDirectory();
        testDir /= "Test.Assets/Gems/PhysX/Code/Tests";
        m_fileIo->SetAlias("@test@", testDir.c_str());

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
    }
} // namespace PhysX
