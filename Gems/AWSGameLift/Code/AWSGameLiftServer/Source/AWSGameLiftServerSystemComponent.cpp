/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
    AWSGameLiftServerSystemComponent::AWSGameLiftServerSystemComponent()
        : m_gameLiftServerManager(AZStd::make_unique<AWSGameLiftServerManager>())
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
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
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
        if (m_gameLiftServerManager->InitializeGameLiftServerSDK())
        {
            GameLiftServerProcessDesc serverProcessDesc;
            UpdateGameLiftServerProcessDesc(serverProcessDesc);
            m_gameLiftServerManager->NotifyGameLiftProcessReady(serverProcessDesc);
        }
    }

    void AWSGameLiftServerSystemComponent::Deactivate()
    {
        m_gameLiftServerManager->HandleDestroySession();
    }

    void AWSGameLiftServerSystemComponent::UpdateGameLiftServerProcessDesc(GameLiftServerProcessDesc& serverProcessDesc)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetDirectInstance();
        if (fileIO)
        {
            const char pathToLogFolder[] = "@log@/";
            char resolvedPath[AZ_MAX_PATH_LEN];
            if (fileIO->ResolvePath(pathToLogFolder, resolvedPath, AZ_ARRAY_SIZE(resolvedPath)))
            {
                serverProcessDesc.m_logPaths.push_back(resolvedPath);
            }
            else
            {
                AZ_Error("AWSGameLift", false, "Failed to resolve the path to the log folder.");
            }
        }
        else
        {
            AZ_Error("AWSGameLift", false, "Failed to get File IO.");
        }

        if (auto console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            AZ::GetValueResult getCvarResult = console->GetCvarValue("sv_port", serverProcessDesc.m_port);
            AZ_Error(
                "AWSGameLift", getCvarResult == AZ::GetValueResult::Success, "Lookup of 'sv_port' console variable failed with error %s",
                AZ::GetEnumString(getCvarResult));
        }
    }

    void AWSGameLiftServerSystemComponent::SetGameLiftServerManager(AZStd::unique_ptr<AWSGameLiftServerManager> gameLiftServerManager)
    {
        m_gameLiftServerManager.reset();
        m_gameLiftServerManager = AZStd::move(gameLiftServerManager);
    }
} // namespace AWSGameLift
