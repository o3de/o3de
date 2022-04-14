/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AutomationApplicationFixture.h>

#include <AutomationSystemComponent.h>

#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Memory/MemoryComponent.h>


namespace UnitTest
{
    namespace
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
    }

    void AutomationApplicationFixture::SetUp()
    {
        AllocatorsFixture::SetUp();
        m_automationComponentDescriptor = Automation::AutomationSystemComponent::CreateDescriptor();
    }

    void AutomationApplicationFixture::TearDown()
    {
        if (m_application)
        {
            DestroyApplication();
        }

        AllocatorsFixture::TearDown();
    }

    AZ::ComponentApplication* AutomationApplicationFixture::CreateApplication(AZStd::vector<const char*>&& args)
    {
        // Create the application.
        m_args = AZStd::move(args);
        m_application = aznew AZ::ComponentApplication(
            aznumeric_cast<int>(m_args.size()),
            const_cast<char**>(m_args.data())
        );

        // Create a system entity.
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_useExistingAllocator = true;

        m_systemEntity = m_application->Create(appDesc);

        m_application->RegisterComponentDescriptor(m_automationComponentDescriptor);

        // Ensure some core components are included on the system entity
        AddComponentIfNotPresent<AZ::MemoryComponent>(m_systemEntity);
        AddComponentIfNotPresent<AZ::AssetManagerComponent>(m_systemEntity);
        AddComponentIfNotPresent<AZ::JobManagerComponent>(m_systemEntity);
        AddComponentIfNotPresent<AZ::StreamerComponent>(m_systemEntity);

        AddComponentIfNotPresent<Automation::AutomationSystemComponent>(m_systemEntity);

        m_systemEntity->Init();
        m_systemEntity->Activate();

        return m_application;
    }

    void AutomationApplicationFixture::DestroyApplication()
    {
        m_application->UnregisterComponentDescriptor(m_automationComponentDescriptor);

        m_application->Destroy();
        delete m_application;
        m_application = nullptr;
    }
} // namespace UnitTest
