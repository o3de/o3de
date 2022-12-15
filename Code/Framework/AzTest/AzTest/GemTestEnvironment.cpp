/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/GemTestEnvironment.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/MemoryComponent.h>

namespace AZ
{
    namespace Test
    {
        // Helper function to avoid having duplicate components
        template<typename T>
        void AddComponentIfNotPresent(AZ::Entity* entity)
        {
            if (entity->FindComponent<T>() == nullptr)
            {
                entity->AddComponent(aznew T());
            }
        }

        /// An application designed to be used in a GemTestEnvironment.
        /// In order to facilitate testing components which are part of a gem, the GemTestApplication can be used to
        /// load only the modules, components etc. which are required to test that gem.
        class GemTestApplication
            : public AZ::ComponentApplication
        {
        public:
            // ComponentApplication
            void SetSettingsRegistrySpecializations(SettingsRegistryInterface::Specializations& specializations) override
            {
                ComponentApplication::SetSettingsRegistrySpecializations(specializations);
                specializations.Append("test");
                specializations.Append("gemtest");
            }
        };

        GemTestEnvironment::Parameters::~Parameters()
        {
            m_componentDescriptors.clear();
            m_dynamicModulePaths.clear();
            m_requiredComponents.clear();
        }

        GemTestEnvironment::GemTestEnvironment()
        {
        }

        void GemTestEnvironment::AddDynamicModulePaths(const AZStd::vector<AZStd::string>& dynamicModulePaths)
        {
            m_parameters->m_dynamicModulePaths.insert(m_parameters->m_dynamicModulePaths.end(),
                dynamicModulePaths.begin(), dynamicModulePaths.end());
        }

        void GemTestEnvironment::AddComponentDescriptors(const AZStd::vector<AZ::ComponentDescriptor*>& componentDescriptors)
        {
            m_parameters->m_componentDescriptors.insert(m_parameters->m_componentDescriptors.end(),
                componentDescriptors.begin(), componentDescriptors.end());
        }

        void GemTestEnvironment::AddRequiredComponents(const AZStd::vector<AZ::TypeId>& requiredComponents)
        {
            m_parameters->m_requiredComponents.insert(m_parameters->m_requiredComponents.end(),
                requiredComponents.begin(), requiredComponents.end());
        }

        AZ::ComponentApplication* GemTestEnvironment::CreateApplicationInstance()
        {
            return aznew GemTestApplication;
        }

        void GemTestEnvironment::SetupEnvironment()
        {
            AZ::AllocatorManager::Instance().SetDefaultTrackingMode(AZ::Debug::AllocationRecords::RECORD_FULL);

            UnitTest::TraceBusHook::SetupEnvironment();

            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

            m_parameters = new Parameters;

            AddGemsAndComponents();
            PreCreateApplication();

            // Create the application.
            m_application = CreateApplicationInstance();
            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_useExistingAllocator = true;

            // Set up gems for loading.
            for (const AZStd::string& dynamicModulePath : m_parameters->m_dynamicModulePaths)
            {
                AZ::DynamicModuleDescriptor dynamicModuleDescriptor;
                dynamicModuleDescriptor.m_dynamicLibraryPath = dynamicModulePath;
                appDesc.m_modules.push_back(dynamicModuleDescriptor);
            }

            // Create a system entity.
            m_systemEntity = m_application->Create(appDesc);

            for (AZ::ComponentDescriptor* descriptor : m_parameters->m_componentDescriptors)
            {
                m_application->RegisterComponentDescriptor(descriptor);
            }

            PostCreateApplication();

            // Some applications (e.g. ToolsApplication) already add some of these components
            // So making sure we don't duplicate them on the system entity.
            AddComponentIfNotPresent<AZ::MemoryComponent>(m_systemEntity);
            AddComponentIfNotPresent<AZ::AssetManagerComponent>(m_systemEntity);
            AddComponentIfNotPresent<AZ::JobManagerComponent>(m_systemEntity);
            AddComponentIfNotPresent<AZ::StreamerComponent>(m_systemEntity);

            m_systemEntity->Init();
            m_systemEntity->Activate();
            PostSystemEntityActivate();

            // Create a separate entity in order to activate this gem's required components.  Note that this assumes
            // any component dependencies are already satisfied either by the system entity or the entities which were
            // created during the module loading above.  It therefore does not do a dependency sort or use the full
            // entity activation.
            m_gemEntity = aznew GemTestEntity();
            for (AZ::TypeId typeId : m_parameters->m_requiredComponents)
            {
                m_gemEntity->CreateComponent(typeId);
            }
            m_gemEntity->Init();
            for (AZ::Component* component : m_gemEntity->GetComponents())
            {
                m_gemEntity->ActivateComponent(*component);
            }

            AZ::AllocatorManager::Instance().GarbageCollect();
        }

        void GemTestEnvironment::TeardownEnvironment()
        {
            for (AZ::ComponentDescriptor* descriptor : m_parameters->m_componentDescriptors)
            {
                m_application->UnregisterComponentDescriptor(descriptor);
            }

            const AZ::Entity::ComponentArrayType& components = m_gemEntity->GetComponents();
            for (auto itComponent = components.rbegin(); itComponent != components.rend(); ++itComponent)
            {
                m_gemEntity->DeactivateComponent(**itComponent);
            }
            delete m_gemEntity;
            m_gemEntity = nullptr;

            PreDestroyApplication();
            m_application->Destroy();
            delete m_application;
            m_application = nullptr;
            PostDestroyApplication();

            delete m_parameters;
            m_parameters = nullptr;

            AZ::GetCurrentSerializeContextModule().Cleanup();

            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();

            UnitTest::TraceBusHook::TeardownEnvironment();
        }
    } // namespace Test
} // namespace AZ
