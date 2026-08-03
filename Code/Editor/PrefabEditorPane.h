/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/CoreLights/DirectionalLightFeatureProcessorInterface.h>
#include <Atom/Feature/Utils/LightingPreset.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <QWidget>

class CLayoutViewPane;

class PrefabEditorPane
    : public QWidget
    , private AZ::TickBus::Handler
    , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
{
    Q_OBJECT
public:
    explicit PrefabEditorPane(QWidget* parent = nullptr);
    ~PrefabEditorPane() override;

    CLayoutViewPane* GetViewPane() const
    {
        return m_viewPane;
    }

private:
    // AZ::TickBus overrides ...
    void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    // AzToolsFramework::EditorEntityContextNotificationBus overrides ...
    void OnViewportWorldChanged(
        AzFramework::ViewportId viewportId, const AzFramework::EntityContextId& worldId) override;

    AzFramework::CameraState GetViewportCameraState() const;
    void LoadLightingPreset();
    void ApplyLightingPreset();

    CLayoutViewPane* m_viewPane = nullptr;

    AzFramework::EntityContextId m_worldId = AzFramework::EntityContextId::CreateNull();
    AZ::Render::LightingPreset m_lightingPreset;
    AZStd::vector<AZ::Render::DirectionalLightFeatureProcessorInterface::LightHandle> m_lightHandles;
};
