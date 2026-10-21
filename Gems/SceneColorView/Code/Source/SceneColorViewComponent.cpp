/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: MIT
 */

#include "SceneColorViewComponent.h"
#include "SceneColorViewFeatureProcessor.h"
#include <Atom/RPI.Public/Scene.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace SceneColorView
{
    bool ApplySettingsForEntity(AZ::EntityId entityId, const Configuration& configuration)
    {
        auto* scene = AZ::RPI::Scene::GetSceneForEntityId(entityId);
        if (!scene) { return true; } // Retry next tick while the scene is being created.
        auto* fp = scene->GetFeatureProcessor<SceneColorViewFeatureProcessor>();
        if (!fp) { fp = scene->EnableFeatureProcessor<SceneColorViewFeatureProcessor>(); }
        return fp ? fp->SetConfiguration(entityId, configuration) : false;
    }
    void ReleaseSettingsForEntity(AZ::EntityId entityId)
    {
        auto* scene = AZ::RPI::Scene::GetSceneForEntityId(entityId);
        if (auto* fp = scene ? scene->GetFeatureProcessor<SceneColorViewFeatureProcessor>() : nullptr)
        { fp->ReleaseConfiguration(entityId); }
    }
    void SceneColorViewComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
        {
            sc->Class<SceneColorViewComponent, AZ::Component>()->Version(0)
                ->Field("Configuration", &SceneColorViewComponent::m_configuration);
        }
    }
    void SceneColorViewComponent::Activate() { AZ::TickBus::Handler::BusConnect(); }
    void SceneColorViewComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        ReleaseSettingsForEntity(GetEntityId());
    }
    void SceneColorViewComponent::OnTick(float, AZ::ScriptTimePoint)
    {
        if (!ApplySettingsForEntity(GetEntityId(), m_configuration) && !m_warned)
        {
            AZ_Warning("SceneColorView", false, "Use only one Scene Color View Capture component per render scene.");
            m_warned = true;
        }
    }
}
