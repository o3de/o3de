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

// LightingPreset.h only forward declares the feature processors it applies to, so the definitions have to
// come from here for Scene::GetFeatureProcessor to resolve them.
#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <AtomToolsFramework/AssetSelection/AssetSelectionComboBox.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include <QToolBar>
#include <QVBoxLayout>

//! The lighting preset a prefab world starts on. Every project has this one: it ships with Atom. The gem
//! alias is what makes it resolvable - a bare product path does not load.
static constexpr const char* DefaultLightingPresetPath =
    "@gemroot:Atom_Feature_Common@/Assets/LightingPresets/default.lightingpreset.azasset";

PrefabEditorPane::PrefabEditorPane(QWidget* parent)
    : QWidget(parent)
{
    m_lightingPresetComboBox = new AtomToolsFramework::AssetSelectionComboBox(
        [](const AZStd::string& path)
        {
            return path.ends_with(AZ::Render::LightingPreset::Extension);
        },
        this);
    m_lightingPresetComboBox->setToolTip(tr("The lighting environment this prefab is shown in. It is not saved into the prefab."));

    connect(
        m_lightingPresetComboBox, &AtomToolsFramework::AssetSelectionComboBox::PathSelected, this,
        [this](const AZStd::string& path)
        {
            LoadLightingPreset(path);
            ApplyLightingPreset();
        });

    auto* toolBar = new QToolBar(this);
    toolBar->addWidget(m_lightingPresetComboBox);

    m_viewPane = new CLayoutViewPane(this);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(toolBar);
    layout->addWidget(m_viewPane, 1);

    LoadLightingPreset(DefaultLightingPresetPath);
    m_lightingPresetComboBox->SelectPath(DefaultLightingPresetPath);

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

void PrefabEditorPane::LoadLightingPreset(const AZStd::string& path)
{
    // The combo box lists catalog paths while presets load from their source file, so a relative path has to be
    // resolved before it will open - the same pairing EntityPreviewViewportToolBar and LoadLightingPreset use.
    AZStd::string resolvedPath = AtomToolsFramework::GetPathWithoutAlias(path);
    if (!resolvedPath.empty() && AZ::IO::PathView(resolvedPath).IsRelative())
    {
        resolvedPath = AtomToolsFramework::GetPathWithoutAlias(AZStd::string::format("@products@/%s", resolvedPath.c_str()));
    }

    auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(resolvedPath);
    if (loadResult && loadResult.GetValue().is<AZ::Render::LightingPreset>())
    {
        m_lightingPreset = AZStd::any_cast<AZ::Render::LightingPreset>(loadResult.GetValue());
        return;
    }

    AZ_Warning("PrefabEditor", false, "Could not load the lighting preset '%s'.", path.c_str());
}

void PrefabEditorPane::ApplyLightingPreset()
{
    AZ::TickBus::Handler::BusDisconnect();
    m_lightHandles.clear();

    // Own-scene-only lookup: AzFramework::Scene::FindSubsystem walks the parent chain and every world scene is a
    // child of Main, so a scene-system query would light the main scene instead of this prefab's.
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

    Camera::Configuration cameraConfig;
    cameraConfig.m_fovRadians = cameraState.m_fovOrZoom;
    cameraConfig.m_nearClipDistance = cameraState.m_nearClip;
    cameraConfig.m_farClipDistance = cameraState.m_farClip;

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
