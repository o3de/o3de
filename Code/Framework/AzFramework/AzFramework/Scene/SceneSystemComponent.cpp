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
#include <AzCore/std/smart_ptr/make_shared.h>
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
    }

    void SceneSystemComponent::Deactivate()
    {
    }

    void SceneSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("SceneSystemComponentService", 0xd8975435));
    }

    void SceneSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("SceneSystemComponentService", 0xd8975435));
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
        m_scenes.push_back(newScene);
        SceneSystemNotificationBus::Broadcast(&SceneSystemNotificationBus::Events::SceneCreated, *newScene);
        return AZ::Success(AZStd::move(newScene));
    }

    AZStd::shared_ptr<Scene> SceneSystemComponent::GetScene(AZStd::string_view name)
    {
        auto sceneIterator = AZStd::find_if(m_scenes.begin(), m_scenes.end(),
            [name](auto& scene) -> bool
            {
                return scene->GetName() == name;
            }
        );

        return sceneIterator == m_scenes.end() ? nullptr : *sceneIterator;
    }

    AZStd::vector<AZStd::shared_ptr<Scene>> SceneSystemComponent::GetAllScenes()
    {
        AZStd::vector<AZStd::shared_ptr<Scene>> scenes;
        scenes.reserve(m_scenes.size());
        
        for (size_t i = 0; i < m_scenes.size(); ++i)
        {
            scenes.push_back(m_scenes[i]);
        }
        return scenes;
    }

    bool SceneSystemComponent::RemoveScene(AZStd::string_view name)
    {
        for (AZStd::shared_ptr<Scene>& scene : m_scenes)
        {
            if (scene->GetName() == name)
            {
                SceneSystemNotificationBus::Broadcast(&SceneSystemNotificationBus::Events::SceneAboutToBeRemoved, *scene);
                
                scene = AZStd::move(m_scenes.back());
                m_scenes.pop_back();
                return true;
            }
        }

        AZ_Warning("SceneSystemComponent", false, R"(Attempting to remove scene name "%.*s", but that scene was not found.)", AZ_STRING_ARG(name));
        return false;
    }

    AZStd::shared_ptr<Scene> SceneSystemComponent::GetSceneFromEntityContextId(EntityContextId entityContextId)
    {
        for (AZStd::shared_ptr<Scene>& scene : m_scenes)
        {
            EntityContext** entityContext = scene->FindSubsystem<EntityContext::SceneStorageType>();
            if (entityContext && (*entityContext)->GetContextId() == entityContextId)
            {
                return scene;
            }
        }
        return nullptr;
    }
} // AzFramework
