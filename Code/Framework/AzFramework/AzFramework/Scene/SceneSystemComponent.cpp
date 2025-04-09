/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Scene/SceneSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

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
                    ;
            }
        }
    }

    SceneSystemComponent::SceneSystemComponent() = default;
    SceneSystemComponent::~SceneSystemComponent() = default;

    void SceneSystemComponent::Activate()
    {
    }

    void SceneSystemComponent::Deactivate()
    {
    }

    void SceneSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("SceneSystemComponentService"));
    }

    void SceneSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("SceneSystemComponentService"));
    }

    AZ::Outcome<AZStd::shared_ptr<Scene>, AZStd::string> SceneSystemComponent::CreateScene(AZStd::string_view name)
    {
        return CreateSceneWithParent(name, nullptr);
    }

    AZ::Outcome<AZStd::shared_ptr<Scene>, AZStd::string> SceneSystemComponent::CreateSceneWithParent(
        AZStd::string_view name, AZStd::shared_ptr<Scene> parent)
    {
        const AZStd::shared_ptr<Scene>& existingScene = GetScene(name);
        if (existingScene)
        {
            return AZ::Failure<AZStd::string>("A scene already exists with this name.");
        }

        auto newScene = AZStd::make_shared<Scene>(name, AZStd::move(parent));
        m_activeScenes.push_back(newScene);
        {
            AZStd::lock_guard lock(m_eventMutex);
            m_events.Signal(EventType::SceneCreated, newScene);
        }
        return AZ::Success(AZStd::move(newScene));
    }

    AZStd::shared_ptr<Scene> SceneSystemComponent::GetScene(AZStd::string_view name)
    {
        auto sceneIterator = AZStd::find_if(m_activeScenes.begin(), m_activeScenes.end(),
            [name](auto& scene) -> bool
            {
                return scene->GetName() == name;
            }
        );

        return sceneIterator == m_activeScenes.end() ? nullptr : *sceneIterator;
    }

    void SceneSystemComponent::IterateActiveScenes(const ActiveIterationCallback& callback)
    {
        bool keepGoing = true;
        auto end = m_activeScenes.end();
        for (auto it = m_activeScenes.begin(); it != end && keepGoing; ++it)
        {
            keepGoing = callback(*it);
        }
    }

    void SceneSystemComponent::IterateZombieScenes(const ZombieIterationCallback& callback)
    {
        bool keepGoing = true;
        auto end = m_zombieScenes.end();
        for (auto it = m_zombieScenes.begin(); it != end && keepGoing;)
        {
            if (!it->expired())
            {
                keepGoing = callback(*(it->lock()));
                ++it;
            }
            else
            {
                *it = m_zombieScenes.back();
                m_zombieScenes.pop_back();
                end = m_zombieScenes.end();
            }
        }
    }

    bool SceneSystemComponent::RemoveScene(AZStd::string_view name)
    {
        for (AZStd::shared_ptr<Scene>& scene : m_activeScenes)
        {
            if (scene->GetName() == name)
            {
                MarkSceneForDestruction(*scene);
                {
                    AZStd::lock_guard lock(m_eventMutex);
                    m_events.Signal(EventType::ScenePendingRemoval, scene);
                }

                // Zombies are weak pointers that are kept around for situations where there's a delay in deleting the scene. This can happen
                // if there are outstanding calls like in-progress async calls or resources locked by hardware. A weak_ptr of the original
                // scene is kept so the zombie scene can still be found through iteration as it may require additional calls such as Tick calls.
                m_zombieScenes.push_back(scene);
                scene = AZStd::move(m_activeScenes.back());
                m_activeScenes.pop_back();
                // The scene may not be held onto anymore, so check here to see if the previously added zombie can be released.
                if (m_zombieScenes.back().expired())
                {
                    m_zombieScenes.pop_back();
                }
                return true;
            }
        }

        AZ_Warning("SceneSystemComponent", false, R"(Attempting to remove scene name "%.*s", but that scene was not found.)", AZ_STRING_ARG(name));
        return false;
    }

    void SceneSystemComponent::ConnectToEvents(SceneEvent::Handler& handler)
    {
        AZStd::lock_guard lock(m_eventMutex);
        handler.Connect(m_events);
    }
} // namespace AzFramework
