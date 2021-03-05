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

#include <AzCore/PlatformDef.h>

#include "EMotionFX_precompiled.h"
#include <MathConversion.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Integration/Assets/ActorAsset.h>
#include <Integration/Editor/Components/EditorSimpleLODComponent.h>


namespace EMotionFX
{
    namespace Integration
    {
        void EditorSimpleLODComponent::Reflect(AZ::ReflectContext* context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<EditorSimpleLODComponent, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(2, VersionConverter)
                    ->Field("LOD Configuration", &EditorSimpleLODComponent::m_configuration)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorSimpleLODComponent>(
                        "Simple LOD Distance (DEPRECATED By Atom)", "This component does not work with Atom renderer. The Simple LOD distance component alters the actor LOD level based on")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Animation (Legacy)")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SimpleLODDistance.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Mannequin.png")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(0, &EditorSimpleLODComponent::m_configuration, "LOD Configuration", "");
                }
            }

        }

        bool EditorSimpleLODComponent::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            const unsigned int version = classElement.GetVersion();
            bool result = true;
            if (version < 2)
            {
                AZ::SerializeContext::DataElementNode LODDistanceNode = classElement.GetSubElement(1);
                classElement.RemoveElement(1);
                result = result && classElement.AddElement<SimpleLODComponent::Configuration>(context, "LOD Configuration");
                AZ::SerializeContext::DataElementNode& LODConfigurationNode = classElement.GetSubElement(1);
                result = result && LODConfigurationNode.AddElement(LODDistanceNode);
            }

            return true;
        }

        EditorSimpleLODComponent::EditorSimpleLODComponent()
            : m_actorInstance(nullptr)
        {
        }

        EditorSimpleLODComponent::~EditorSimpleLODComponent()
        {
        }

        void EditorSimpleLODComponent::Activate()
        {
            EMotionFXPtr<EMotionFX::ActorInstance> actorInstance;
            ActorComponentRequestBus::EventResult(
                actorInstance,
                GetEntityId(),
                &ActorComponentRequestBus::Events::GetActorInstance);

            if (actorInstance)
            {
                m_actorInstance = actorInstance.get();
                const AZ::u32 numLODs = m_actorInstance->GetActor()->GetNumLODLevels();
                m_configuration.GenerateDefaultValue(numLODs);
            }
            else
            {
                m_actorInstance = nullptr;
            }

            ActorComponentNotificationBus::Handler::BusConnect(GetEntityId());
            AZ::TickBus::Handler::BusConnect();
        }

        void EditorSimpleLODComponent::Deactivate()
        {
            AZ::TickBus::Handler::BusDisconnect();
            ActorComponentNotificationBus::Handler::BusDisconnect();
        }

        void EditorSimpleLODComponent::OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance)
        {
            if (m_actorInstance != actorInstance)
            {
                m_actorInstance = actorInstance;
                const AZ::u32 numLODs = m_actorInstance->GetActor()->GetNumLODLevels();
                m_configuration.GenerateDefaultValue(numLODs);
            }
        }

        void EditorSimpleLODComponent::OnActorInstanceDestroyed([[maybe_unused]] EMotionFX::ActorInstance* actorInstance)
        {
            m_actorInstance = nullptr;
            m_configuration.Reset();
        }

        void EditorSimpleLODComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            SimpleLODComponent::UpdateLodLevelByDistance(m_actorInstance, m_configuration, GetEntityId());
        }

        void EditorSimpleLODComponent::BuildGameEntity(AZ::Entity* gameEntity)
        {
            gameEntity->AddComponent(aznew SimpleLODComponent(&m_configuration));
        }
    }
}
