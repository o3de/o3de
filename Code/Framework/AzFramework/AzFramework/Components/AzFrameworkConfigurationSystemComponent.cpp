/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/AzFrameworkConfigurationSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Debug/Trace.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>

namespace AzFramework
{
    void AzFrameworkConfigurationSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AzFrameworkConfigurationSystemComponent, AZ::Component>();

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AzFrameworkConfigurationSystemComponent>(
                    "AzFramework Configuration Component", "System component responsible for configuring AzFramework")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Method("Terminate", &AZ::Debug::Trace::Terminate, nullptr, "Terminates the process with the specified exit code")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "framework")
                ;
        }
    }

    void AzFrameworkConfigurationSystemComponent::Activate()
    {
        AZ::Outcome<AZStd::shared_ptr<Scene>, AZStd::string> createSceneOutcome =
            SceneSystemInterface::Get()->CreateScene(Scene::MainSceneName);
        if (createSceneOutcome)
        {
            AZStd::shared_ptr<Scene> scene = createSceneOutcome.TakeValue();
            EntityContext* gameEntityContext = nullptr;
            GameEntityContextRequestBus::BroadcastResult(gameEntityContext, &GameEntityContextRequests::GetGameEntityContextInstance);
            if (gameEntityContext != nullptr)
            {
                [[maybe_unused]] bool result = scene->SetSubsystem<EntityContext::SceneStorageType&>(gameEntityContext);
                AZ_Assert(result, "Unable to register main entity context with the main scene.");
            }
            else
            {
                AZ_Assert(false, "Unable to retrieve the game entity context instance.");
            }
        }
        else
        {
            AZ_Assert(false, "Unable to create main scene due to: %s", createSceneOutcome.GetError().c_str());
        }

    }

    void AzFrameworkConfigurationSystemComponent::Deactivate()
    {
        [[maybe_unused]] bool success = SceneSystemInterface::Get()->RemoveScene(Scene::MainSceneName);
        AZ_Assert(success, "Unable to remove the main scene.");
    }

    void AzFrameworkConfigurationSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AzFrameworkConfigurationSystemComponentService"));
    }

    void AzFrameworkConfigurationSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AzFrameworkConfigurationSystemComponentService"));
    }

    void AzFrameworkConfigurationSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("SceneSystemComponentService"));
        dependent.push_back(AZ_CRC_CE("GameEntityContextService"));
    }

} // AzFramework
