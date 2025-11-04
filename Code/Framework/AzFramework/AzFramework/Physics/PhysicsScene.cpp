/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/Physics/PhysicsScene.h>

#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/BehaviorContext.h>


namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(Scene, AZ::SystemAllocator);

    /*static*/ void Scene::Reflect(AZ::ReflectContext* context)
    {
        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            const auto getOnGravityChange = [](const AZStd::string& sceneName) -> SceneEvents::OnSceneGravityChangedEvent*
            {
                if (auto* physicsSystem = AZ::Interface<SystemInterface>::Get())
                {
                    SceneHandle sceneHandle = physicsSystem->GetSceneHandle(sceneName);
                    if (sceneHandle != AzPhysics::InvalidSceneHandle)
                    {
                        if (Scene* scene = physicsSystem->GetScene(sceneHandle))
                        {
                            return scene->GetOnGravityChangedEvent();
                        }
                    }
                }
                return nullptr;
            };
            const AZ::BehaviorAzEventDescription gravityChangedEventDescription =
            {
                "On Gravity Changed event",
                {
                    "Scene Handle",
                    "Gravity Vector"
                } // Parameters
            };

            behaviorContext->Class<Scene>("PhysicsScene")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "Physics")
                ->Method("GetOnGravityChangeEvent", getOnGravityChange)
                    ->Attribute(AZ::Script::Attributes::AzEventDescription, gravityChangedEventDescription)
                ->Method("QueryScene", [](Scene* self, const SceneQueryRequest* request)
                {
                    return self->QueryScene(request);
                })
                ;
        }
    }

    Scene::Scene(const SceneConfiguration& config)
        : m_id(config.m_sceneName)
    {

    }

    const AZ::Crc32& Scene::GetId() const
    {
        return m_id;
    }

    SceneEvents::OnSceneGravityChangedEvent* Scene::GetOnGravityChangedEvent()
    {
        return &m_sceneGravityChangedEvent;
    }
} // namespace AzPhysics
