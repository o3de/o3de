/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
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
            AZStd::unordered_map<AZ::Render::DisplayMapperOperationType, QString> operationNameMap = {
                { AZ::Render::DisplayMapperOperationType::Reinhard, "Reinhard" },
                { AZ::Render::DisplayMapperOperationType::GammaSRGB, "GammaSRGB" },
                { AZ::Render::DisplayMapperOperationType::Passthrough, "Passthrough" },
                { AZ::Render::DisplayMapperOperationType::AcesLut, "AcesLut" },
                { AZ::Render::DisplayMapperOperationType::Aces, "Aces" }
            };

            AZ::Render::DisplayMapperOperationType currentOperationType = {};
            EntityPreviewViewportSettingsRequestBus::EventResult(
                currentOperationType, m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::GetDisplayMapperOperationType);

            QMenu menu;
            for (auto operationNamePair : operationNameMap)
            {
                auto operationAction = menu.addAction(operationNamePair.second, [this, operationNamePair]() {
                    EntityPreviewViewportSettingsRequestBus::Event(
                        m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SetDisplayMapperOperationType,
                        operationNamePair.first);
                });
                operationAction->setCheckable(true);
                operationAction->setChecked(currentOperationType==operationNamePair.first);
            }
            menu.exec(QCursor::pos());
        });
        displayMapperAction->setCheckable(false);

        // Add lighting preset combo box
        m_lightingPresetComboBox = new AssetSelectionComboBox([](const AZStd::string& path) {
            return path.ends_with(AZ::Render::LightingPreset::Extension);
        }, this);
        connect(m_lightingPresetComboBox, &AssetSelectionComboBox::PathSelected, this, [this](const AZStd::string& path) {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadLightingPreset, path);
        });
        addWidget(m_lightingPresetComboBox);

        // Add model preset combo box
        m_modelPresetComboBox = new AssetSelectionComboBox([](const AZStd::string& path) {
            return path.ends_with(AZ::Render::ModelPreset::Extension);
        }, this);
        connect(m_modelPresetComboBox, &AssetSelectionComboBox::PathSelected, this, [this](const AZStd::string& path) {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadModelPreset, path);
        });
        addWidget(m_modelPresetComboBox);

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
} // namespace AtomToolsFramework

#include <AtomToolsFramework/EntityPreviewViewport/moc_EntityPreviewViewportToolBar.cpp>
