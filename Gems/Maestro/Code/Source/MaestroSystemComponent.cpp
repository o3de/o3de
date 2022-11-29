/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <ISystem.h>

#include "Cinematics/Movie.h"

#include "MaestroSystemComponent.h"

namespace Maestro
{
    void MaestroAllocatorComponent::Activate()
    {
    }

    void MaestroAllocatorComponent::Deactivate()
    {
    }

    void MaestroAllocatorComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("MemoryAllocators", 0xd59acbcc));
    }

    void MaestroAllocatorComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<MaestroAllocatorComponent, AZ::Component>()->Version(1)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC("AssetBuilder", 0xc739c7d7) }));
        }
    }

    void MaestroSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaestroSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<MaestroSystemComponent>("Maestro", "Provides the Open 3D Engine Cinematics Service")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        // ->Attribute(AZ::Edit::Attributes::Category, "") Set a category
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void MaestroSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("MaestroService", 0xba05938e));
    }

    void MaestroSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("MaestroService", 0xba05938e));
    }

    void MaestroSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("MemoryAllocators", 0xd59acbcc));
    }

    void MaestroSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void MaestroSystemComponent::Init()
    {
    }

    void MaestroSystemComponent::Activate()
    {
        MaestroRequestBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
    }

    void MaestroSystemComponent::Deactivate()
    {
        MaestroRequestBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void MaestroSystemComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& startupParams)
    {
        if (!startupParams.bSkipMovie)
        {
            // Create the movie System
            m_movieSystem.reset(new CMovieSystem(&system));
            gEnv->pMovieSystem = m_movieSystem.get();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void MaestroSystemComponent::OnCrySystemShutdown([[maybe_unused]] ISystem& system)
    {
        if (gEnv && gEnv->pMovieSystem)
        {
            gEnv->pMovieSystem = nullptr;
            // delete m_movieSystem
            m_movieSystem.reset();
        }
    }
}
