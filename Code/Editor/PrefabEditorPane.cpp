/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "PrefabEditorPane.h"

#include "ViewPane.h"

// LightingPreset.h only forward declares these, but Scene::GetFeatureProcessor needs the definitions.
#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include <QVBoxLayout>

//! The lighting preset a prefab world is shown in. The gem alias is what makes it resolvable.
static constexpr const char* DefaultLightingPresetPath =
    "@gemroot:Atom_Feature_Common@/Assets/LightingPresets/default.lightingpreset.azasset";

PrefabEditorPane::PrefabEditorPane(QWidget* parent)
    : QWidget(parent)
{
    m_viewPane = new CLayoutViewPane(this);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_viewPane, 1);

    LoadLightingPreset();

    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
}

PrefabEditorPane::~PrefabEditorPane()
{
    AZ::TickBus::Handler::BusDisconnect();
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
}

void PrefabEditorPane::OnViewportWorldChanged(
    AzFramework::ViewportId viewportId, const AzFramework::EntityContextId& worldId)
{
    if (!m_viewPane || viewportId != m_viewPane->GetId())
    {
        return;
    }

    m_worldId = worldId;
    ApplyLightingPreset();
}

void PrefabEditorPane::LoadLightingPreset()
{
    auto loadResult =
        AZ::JsonSerializationUtils::LoadAnyObjectFromFile(AtomToolsFramework::GetPathWithoutAlias(DefaultLightingPresetPath));
    if (loadResult && loadResult.GetValue().is<AZ::Render::LightingPreset>())
    {
        m_lightingPreset = AZStd::any_cast<AZ::Render::LightingPreset>(loadResult.GetValue());
        return;
    }

    AZ_Warning("PrefabEditor", false, "Could not load the lighting preset '%s'.", DefaultLightingPresetPath);
}

void PrefabEditorPane::ApplyLightingPreset()
{
    AZ::TickBus::Handler::BusDisconnect();
    m_lightHandles.clear();

    // FindSubsystem walks up to Main, so query this scene only or the main scene gets lit instead.
    AZStd::shared_ptr<AzFramework::Scene> worldScene;
    AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
        worldScene, &AzToolsFramework::EditorEntityContextRequests::GetWorldScene, m_worldId);
    AZ::RPI::ScenePtr* renderScene = worldScene ? worldScene->FindSubsystemInScene<AZ::RPI::ScenePtr>() : nullptr;
    if (!renderScene || !*renderScene)
    {
        return;
    }

    AZ::RPI::Scene* scene = renderScene->get();
    auto* imageBasedLightFeatureProcessor = scene->GetFeatureProcessor<AZ::Render::ImageBasedLightFeatureProcessorInterface>();
    auto* skyBoxFeatureProcessor = scene->GetFeatureProcessor<AZ::Render::SkyBoxFeatureProcessorInterface>();
    auto* directionalLightFeatureProcessor = scene->GetFeatureProcessor<AZ::Render::DirectionalLightFeatureProcessorInterface>();
    auto* postProcessFeatureProcessor = scene->GetFeatureProcessor<AZ::Render::PostProcessFeatureProcessorInterface>();
    if (!imageBasedLightFeatureProcessor || !skyBoxFeatureProcessor || !directionalLightFeatureProcessor ||
        !postProcessFeatureProcessor)
    {
        return;
    }

    // The exposure settings belong to the scene rather than to an entity, as the asset thumbnail renderer also does.
    auto* exposureControlSettingInterface =
        postProcessFeatureProcessor->GetOrCreateSettingsInterface(AZ::EntityId())->GetOrCreateExposureControlSettingsInterface();

    const AzFramework::CameraState cameraState = GetViewportCameraState();

    // The frustum extents give the configuration its aspect ratio; zero makes cascade fitting reject it.
    Camera::Configuration cameraConfig;
    cameraConfig.m_fovRadians = cameraState.m_fovOrZoom;
    cameraConfig.m_nearClipDistance = cameraState.m_nearClip;
    cameraConfig.m_farClipDistance = cameraState.m_farClip;
    cameraConfig.m_frustumWidth = AZStd::max(aznumeric_cast<float>(cameraState.m_viewportSize.m_width), 1.0f);
    cameraConfig.m_frustumHeight = AZStd::max(aznumeric_cast<float>(cameraState.m_viewportSize.m_height), 1.0f);

    m_lightingPreset.ApplyLightingPreset(
        imageBasedLightFeatureProcessor, skyBoxFeatureProcessor, exposureControlSettingInterface,
        directionalLightFeatureProcessor, cameraConfig, m_lightHandles, false);

    // Shadow cascades are fitted to the camera, so they have to follow it.
    AZ::TickBus::Handler::BusConnect();
}

AzFramework::CameraState PrefabEditorPane::GetViewportCameraState() const
{
    AzFramework::CameraState cameraState;
    if (m_viewPane)
    {
        AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::EventResult(
            cameraState, m_viewPane->GetId(),
            &AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Events::GetCameraState);
    }
    return cameraState;
}

void PrefabEditorPane::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
{
    if (m_lightHandles.empty())
    {
        return;
    }

    AZStd::shared_ptr<AzFramework::Scene> worldScene;
    AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
        worldScene, &AzToolsFramework::EditorEntityContextRequests::GetWorldScene, m_worldId);
    AZ::RPI::ScenePtr* renderScene = worldScene ? worldScene->FindSubsystemInScene<AZ::RPI::ScenePtr>() : nullptr;
    auto* directionalLightFeatureProcessor = renderScene && *renderScene
        ? renderScene->get()->GetFeatureProcessor<AZ::Render::DirectionalLightFeatureProcessorInterface>()
        : nullptr;
    if (!directionalLightFeatureProcessor)
    {
        return;
    }

    const AzFramework::CameraState cameraState = GetViewportCameraState();
    const AZ::Transform cameraTransform = AZ::Transform::CreateFromMatrix3x3AndTranslation(
        AZ::Matrix3x3::CreateFromColumns(cameraState.m_side, cameraState.m_forward, cameraState.m_up),
        cameraState.m_position);

    for (const auto& lightHandle : m_lightHandles)
    {
        directionalLightFeatureProcessor->SetCameraTransform(lightHandle, cameraTransform);
    }
}
