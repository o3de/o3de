/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <Atom/RPI.Reflect/System/RenderPipelineDescriptor.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsRequestBus.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportToolBar.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/std/containers/vector.h>
#include <AzQtComponents/Components/Widgets/ToolBar.h>

#include <QAbstractItemView>
#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QToolButton>
#include <QtConcurrent/QtConcurrent>

namespace AtomToolsFramework
{
    EntityPreviewViewportToolBar::EntityPreviewViewportToolBar(const AZ::Crc32& toolId, QWidget* parent)
        : QToolBar(parent)
        , m_toolId(toolId)
    {
        setObjectName("EntityPreviewViewportToolBar");

        AzQtComponents::ToolBar::addMainToolBarStyle(this);

        // Add toggle grid button
        m_toggleGrid = addAction(QIcon(":/Icons/grid.svg"), "Toggle Grid");
        m_toggleGrid->setCheckable(true);
        connect(m_toggleGrid, &QAction::triggered, [this]() {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SetGridEnabled, m_toggleGrid->isChecked());
        });

        // Add toggle shadow catcher button
        m_toggleShadowCatcher = addAction(QIcon(":/Icons/shadow.svg"), "Toggle Shadow Catcher");
        m_toggleShadowCatcher->setCheckable(true);
        connect(m_toggleShadowCatcher, &QAction::triggered, [this]() {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SetShadowCatcherEnabled, m_toggleShadowCatcher->isChecked());
        });

        // Add toggle alternate skybox button
        m_toggleAlternateSkybox = addAction(QIcon(":/Icons/skybox.svg"), "Toggle Alternate Skybox");
        m_toggleAlternateSkybox->setCheckable(true);
        connect(m_toggleAlternateSkybox, &QAction::triggered, [this]() {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SetAlternateSkyboxEnabled, m_toggleAlternateSkybox->isChecked());
        });

        // Add mapping selection button
        auto displayMapperAction = addAction(QIcon(":/Icons/toneMapping.svg"), "Tone Mapping", this, [this]() {
            AZ::Render::DisplayMapperOperationType currentOperationType = {};
            EntityPreviewViewportSettingsRequestBus::EventResult(
                currentOperationType, m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::GetDisplayMapperOperationType);

            QMenu menu;
            for (const auto& operationEnumPair : AZ::AzEnumTraits<AZ::Render::DisplayMapperOperationType>::Members)
            {
                auto operationAction = menu.addAction(operationEnumPair.m_string.data(), [this, operationEnumPair]() {
                    EntityPreviewViewportSettingsRequestBus::Event(
                        m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SetDisplayMapperOperationType,
                        operationEnumPair.m_value);
                });
                operationAction->setCheckable(true);
                operationAction->setChecked(currentOperationType == operationEnumPair.m_value);
            }
            menu.exec(QCursor::pos());
        });
        displayMapperAction->setCheckable(false);

        // Setting the minimum drop down with for all asset selection combo boxes to compensate for longer file names, like render
        // pipelines.
        const int minComboBoxDropdownWidth = 220;

        // Add lighting preset combo box
        m_lightingPresetComboBox = new AssetSelectionComboBox([](const AZStd::string& path) {
            return path.ends_with(AZ::Render::LightingPreset::Extension);
        }, this);
        m_lightingPresetComboBox->view()->setMinimumWidth(minComboBoxDropdownWidth);
        addWidget(m_lightingPresetComboBox);

        // Add model preset combo box
        m_modelPresetComboBox = new AssetSelectionComboBox([](const AZStd::string& path) {
            return path.ends_with(AZ::Render::ModelPreset::Extension);
        }, this);
        m_modelPresetComboBox->view()->setMinimumWidth(minComboBoxDropdownWidth);
        addWidget(m_modelPresetComboBox);

        // Add render pipeline combo box
        m_renderPipelineComboBox = new AssetSelectionComboBox([](const AZStd::string& path) {
            return path.ends_with(AZ::RPI::RenderPipelineDescriptor::Extension);
        }, this);
        m_renderPipelineComboBox->view()->setMinimumWidth(minComboBoxDropdownWidth);
        addWidget(m_renderPipelineComboBox);
        
        // Prepopulating preset selection widgets with previously registered presets.
        EntityPreviewViewportSettingsRequestBus::Event(
            m_toolId,
            [this](EntityPreviewViewportSettingsRequests* viewportRequests)
            {
                m_lightingPresetComboBox->AddPath(viewportRequests->GetLastLightingPresetPath());
                for (const auto& path : viewportRequests->GetRegisteredLightingPresetPaths())
                {
                    m_lightingPresetComboBox->AddPath(path);
                }

                m_modelPresetComboBox->AddPath(viewportRequests->GetLastModelPresetPath());
                for (const auto& path : viewportRequests->GetRegisteredModelPresetPaths())
                {
                    m_modelPresetComboBox->AddPath(path);
                }
                
                m_renderPipelineComboBox->AddPath(viewportRequests->GetLastRenderPipelinePath());
                for (const auto& path : viewportRequests->GetRegisteredRenderPipelinePaths())
                {
                    m_renderPipelineComboBox->AddPath(path);
                }
            });

        connect(m_lightingPresetComboBox, &AssetSelectionComboBox::PathSelected, this, [this](const AZStd::string& path) {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadLightingPreset, path);
        });

        connect(m_modelPresetComboBox, &AssetSelectionComboBox::PathSelected, this, [this](const AZStd::string& path) {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadModelPreset, path);
        });

        connect(m_renderPipelineComboBox, &AssetSelectionComboBox::PathSelected, this, [this](const AZStd::string& path) {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadRenderPipeline, path);
        });
 
        OnViewportSettingsChanged();
        EntityPreviewViewportSettingsNotificationBus::Handler::BusConnect(m_toolId);
    }

    EntityPreviewViewportToolBar::~EntityPreviewViewportToolBar()
    {
        EntityPreviewViewportSettingsNotificationBus::Handler::BusDisconnect();
    }

    void EntityPreviewViewportToolBar::OnViewportSettingsChanged()
    {
        EntityPreviewViewportSettingsRequestBus::Event(
            m_toolId,
            [this](EntityPreviewViewportSettingsRequests* viewportRequests)
            {
                m_toggleGrid->setChecked(viewportRequests->GetGridEnabled());
                m_toggleShadowCatcher->setChecked(viewportRequests->GetShadowCatcherEnabled());
                m_toggleAlternateSkybox->setChecked(viewportRequests->GetAlternateSkyboxEnabled());
                m_lightingPresetComboBox->SelectPath(viewportRequests->GetLastLightingPresetPath());
                m_modelPresetComboBox->SelectPath(viewportRequests->GetLastModelPresetPath());
                m_renderPipelineComboBox->SelectPath(viewportRequests->GetLastRenderPipelinePath());
            });
    }

    void EntityPreviewViewportToolBar::OnModelPresetAdded(const AZStd::string& path)
    {
        m_modelPresetComboBox->AddPath(path);
    }

    void EntityPreviewViewportToolBar::OnLightingPresetAdded(const AZStd::string& path)
    {
        m_lightingPresetComboBox->AddPath(path);
    }

    void EntityPreviewViewportToolBar::OnRenderPipelineAdded(const AZStd::string& path)
    {
        m_renderPipelineComboBox->AddPath(path);
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/EntityPreviewViewport/moc_EntityPreviewViewportToolBar.cpp>
