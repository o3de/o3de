/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSGameLiftServerSystemComponent.h>
#include <AWSGameLiftServerManager.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>

namespace AWSGameLift
{
    AZ_CVAR(bool, sv_gameLiftEnabled, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Activate GameLift server manager and SDK");

    AWSGameLiftServerSystemComponent::AWSGameLiftServerSystemComponent()
    {
    }

    AWSGameLiftServerSystemComponent::~AWSGameLiftServerSystemComponent()
    {
        m_gameLiftServerManager.reset();
    }

    void AWSGameLiftServerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AWSGameLiftServerSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AWSGameLiftServerSystemComponent>("AWSGameLiftServer", "Create the GameLift server manager which manages the server process for hosting a game session via GameLiftServerSDK.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AWSGameLiftServerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AWSGameLiftServerService"));
    }

    void AWSGameLiftServerSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AWSGameLiftServerService"));
    }

    void AWSGameLiftServerSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void AWSGameLiftServerSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AWSGameLiftServerSystemComponent::Init()
    {
    }

    void AWSGameLiftServerSystemComponent::Activate()
    {
        if (sv_gameLiftEnabled)
        {
            m_gameLiftServerManager = AZStd::make_unique<AWSGameLiftServerManager>();
            m_gameLiftServerManager->InitializeGameLiftServerSDK();
            m_gameLiftServerManager->ActivateManager();
        }
    }

    void AWSGameLiftServerSystemComponent::Deactivate()
    {
        if (m_gameLiftServerManager)
        {
            m_gameLiftServerManager->DeactivateManager();
            m_gameLiftServerManager->HandleDestroySession();
            m_gameLiftServerManager.reset();
        }
    }

    void AWSGameLiftServerSystemComponent::SetGameLiftServerManager(AZStd::unique_ptr<AWSGameLiftServerManager> gameLiftServerManager)
    {
        m_gameLiftServerManager.reset();
        m_gameLiftServerManager = AZStd::move(gameLiftServerManager);
    }
} // namespace AWSGameLift
