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

    AZ::Outcome<Scene*, AZStd::string> SceneSystemComponent::CreateScene(AZStd::string_view name)
    {
        return CreateSceneWithParent(name, nullptr);
    }

    AZ::Outcome<Scene*, AZStd::string> SceneSystemComponent::CreateSceneWithParent(AZStd::string_view name, Scene* parent)
    {
        Scene* existingScene = GetScene(name);

        if (existingScene)
        {
            return AZ::Failure<AZStd::string>("A scene already exists with this name.");
        }

        auto newScene = AZStd::make_unique<Scene>(name, parent);
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
        for (AZStd::unique_ptr<Scene>& scene : m_scenes)
        {
            if (scene->GetName() == name)
            {
                SceneSystemNotificationBus::Broadcast(&SceneSystemNotificationBus::Events::SceneAboutToBeRemoved, *scene);
                SceneNotificationBus::Event(scene.get(), &SceneNotificationBus::Events::SceneAboutToBeRemoved);

                scene = AZStd::move(m_scenes.back());
                m_scenes.pop_back();
                return true;
            }
        }

        AZ_Warning("SceneSystemComponent", false, R"(Attempting to remove scene name "%.*s", but that scene was not found.)", AZ_STRING_ARG(name));
        return false;
    }

    Scene* SceneSystemComponent::GetSceneFromEntityContextId(EntityContextId entityContextId)
    {
        for (AZStd::unique_ptr<Scene>& scene : m_scenes)
        {
            EntityContext** entityContext = scene->FindSubsystem<EntityContext::SceneStorageType>();
            if (entityContext && (*entityContext)->GetContextId() == entityContextId)
            {
                return scene.get();
            }
        }
        return nullptr;
    }
} // AzFramework
