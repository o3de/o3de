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

#include <AzFramework/Scene/SceneSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Scene/Scene.h>

namespace AzFramework
{
    void SceneSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SceneSystemComponent, AZ::Component>();

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SceneSystemComponent>(
                    "Scene System Component", "System component responsible for owning scenes")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }
        }
    }

    SceneSystemComponent::SceneSystemComponent() = default;
    SceneSystemComponent::~SceneSystemComponent() = default;

    void SceneSystemComponent::Activate()
    {
        // Connect busses
        SceneSystemRequestBus::Handler::BusConnect();
    }

    void SceneSystemComponent::Deactivate()
    {
        // Disconnect Busses
        SceneSystemRequestBus::Handler::BusDisconnect();
    }

    void SceneSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("SceneSystemComponentService", 0xd8975435));
    }

    void SceneSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("SceneSystemComponentService", 0xd8975435));
    }

    AZ::Outcome<Scene*, AZStd::string> SceneSystemComponent::CreateScene(AZStd::string_view name)
    {
        Scene* existingScene = GetScene(name);

        if (existingScene)
        {
            return AZ::Failure<AZStd::string>("A scene already exists with this name.");
        }

        auto newScene = AZStd::make_unique<Scene>(name);
        Scene* scenePointer = newScene.get();
        m_scenes.push_back(AZStd::move(newScene));
        SceneSystemNotificationBus::Broadcast(&SceneSystemNotificationBus::Events::SceneCreated, *scenePointer);
        return AZ::Success(scenePointer);
    }

    Scene* SceneSystemComponent::GetScene(AZStd::string_view name)
    {
        auto sceneIterator = AZStd::find_if(m_scenes.begin(), m_scenes.end(),
            [name](auto& scene) -> bool
            {
                return scene->GetName() == name;
            }
        );

        return sceneIterator == m_scenes.end() ? nullptr : sceneIterator->get();
    }

    AZStd::vector<Scene*> SceneSystemComponent::GetAllScenes()
    {
        AZStd::vector<Scene*> scenes;
        scenes.resize_no_construct(m_scenes.size());

        for (size_t i = 0; i < m_scenes.size(); ++i)
        {
            scenes.at(i) = m_scenes.at(i).get();
        }
        return scenes;
    }

    bool SceneSystemComponent::RemoveScene(AZStd::string_view name)
    {
        for (size_t i = 0; i < m_scenes.size(); ++i)
        {
            auto& scenePtr = m_scenes.at(i);
            if (scenePtr->GetName() == name)
            {
                // Remove any entityContext mappings.
                Scene* scene = scenePtr.get();
                for (auto entityContextScenePairIt = m_entityContextToScenes.begin(); entityContextScenePairIt != m_entityContextToScenes.end();)
                {
                    AZStd::pair<EntityContextId, Scene*>& pair = *entityContextScenePairIt;
                    if (pair.second == scene)
                    {
                        // swap and pop back.
                        *entityContextScenePairIt = m_entityContextToScenes.back();
                        m_entityContextToScenes.pop_back();
                    }
                    else
                    {
                        ++entityContextScenePairIt;
                    }
                }

                SceneSystemNotificationBus::Broadcast(&SceneSystemNotificationBus::Events::SceneAboutToBeRemoved, *scene);
                SceneNotificationBus::Event(scene, &SceneNotificationBus::Events::SceneAboutToBeRemoved);

                m_scenes.erase(&scenePtr);
                return true;
            }
        }

        AZ_Warning("SceneSystemComponent", false, "Attempting to remove scene name \"%.*s\", but that scene was not found.", static_cast<int>(name.size()), name.data());
        return false;
    }

    bool SceneSystemComponent::SetSceneForEntityContextId(EntityContextId entityContextId, Scene* scene)
    {
        Scene* existingSceneForEntityContext = GetSceneFromEntityContextId(entityContextId);
        if (existingSceneForEntityContext)
        {
            // This entity context is already mapped and must be unmapped explictely before it can be changed.
            char entityContextIdString[EntityContextId::MaxStringBuffer];
            entityContextId.ToString(entityContextIdString, sizeof(entityContextIdString));
            AZ_Warning("SceneSystemComponent", false, "Failed to set a scene for entity context %s, scene is already set for that entity context.", entityContextIdString);

            return false;
        }
        m_entityContextToScenes.emplace_back(entityContextId, scene);
        SceneNotificationBus::Event(scene, &SceneNotificationBus::Events::EntityContextMapped, entityContextId);
        return true;
    }

    bool SceneSystemComponent::RemoveSceneForEntityContextId(EntityContextId entityContextId, Scene* scene)
    {
        if (!scene || entityContextId.IsNull())
        {
            return false;
        }

        for (auto entityContextScenePairIt = m_entityContextToScenes.begin(); entityContextScenePairIt != m_entityContextToScenes.end();)
        {
            AZStd::pair<EntityContextId, Scene*>& pair = *entityContextScenePairIt;
            if (!(pair.first == entityContextId && pair.second == scene))
            {
                ++entityContextScenePairIt;
            }
            else
            {
                // swap and pop back.
                *entityContextScenePairIt = m_entityContextToScenes.back();
                m_entityContextToScenes.pop_back();

                SceneNotificationBus::Event(scene, &SceneNotificationBus::Events::EntityContextUnmapped, entityContextId);
                return true;
            }
        }

        char entityContextIdString[EntityContextId::MaxStringBuffer];
        entityContextId.ToString(entityContextIdString, sizeof(entityContextIdString));
        AZ_Warning("SceneSystemComponent", false, "Failed to remove scene \"%.*s\" for entity context %s, entity context is not currently mapped to that scene.", static_cast<int>(scene->GetName().size()), scene->GetName().data(), entityContextIdString);
        return false;
    }

    Scene* SceneSystemComponent::GetSceneFromEntityContextId(EntityContextId entityContextId)
    {
        for (AZStd::pair<EntityContextId, Scene*>& pair : m_entityContextToScenes)
        {
            if (pair.first == entityContextId)
            {
                return pair.second;
            }
        }
        return nullptr;
    }

} // AzFramework