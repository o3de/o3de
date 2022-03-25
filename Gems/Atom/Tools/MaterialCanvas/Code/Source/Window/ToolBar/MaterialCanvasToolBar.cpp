/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/std/containers/vector.h>
#include <Viewport/MaterialCanvasViewportSettingsNotificationBus.h>
#include <Viewport/MaterialCanvasViewportSettingsRequestBus.h>
#include <Window/ToolBar/MaterialCanvasToolBar.h>

#include <AzQtComponents/Components/Widgets/ToolBar.h>
#include <QAbstractItemView>
#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QToolButton>

namespace MaterialCanvas
{
    MaterialCanvasToolBar::MaterialCanvasToolBar(const AZ::Crc32& toolId, QWidget* parent)
        : QToolBar(parent)
        , m_toolId(toolId)
    {
        AzQtComponents::ToolBar::addMainToolBarStyle(this);

        // Add toggle grid button
        m_toggleGrid = addAction(QIcon(":/Icons/grid.svg"), "Toggle Grid");
        m_toggleGrid->setCheckable(true);
        connect(m_toggleGrid, &QAction::triggered, [this]() {
            MaterialCanvasViewportSettingsRequestBus::Event(
                m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::SetGridEnabled, m_toggleGrid->isChecked());
        });

        // Add toggle shadow catcher button
        m_toggleShadowCatcher = addAction(QIcon(":/Icons/shadow.svg"), "Toggle Shadow Catcher");
        m_toggleShadowCatcher->setCheckable(true);
        connect(m_toggleShadowCatcher, &QAction::triggered, [this]() {
            MaterialCanvasViewportSettingsRequestBus::Event(
                m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::SetShadowCatcherEnabled, m_toggleShadowCatcher->isChecked());
        });

        // Add toggle alternate skybox button
        m_toggleAlternateSkybox = addAction(QIcon(":/Icons/skybox.svg"), "Toggle Alternate Skybox");
        m_toggleAlternateSkybox->setCheckable(true);
        connect(m_toggleAlternateSkybox, &QAction::triggered, [this]() {
            MaterialCanvasViewportSettingsRequestBus::Event(
                m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::SetAlternateSkyboxEnabled, m_toggleAlternateSkybox->isChecked());
        });

        // Add mapping selection button
        QToolButton* toneMappingButton = new QToolButton(this);
        QMenu* toneMappingMenu = new QMenu(toneMappingButton);

        m_operationNames = {
            { AZ::Render::DisplayMapperOperationType::Reinhard, "Reinhard" },
            { AZ::Render::DisplayMapperOperationType::GammaSRGB, "GammaSRGB" },
            { AZ::Render::DisplayMapperOperationType::Passthrough, "Passthrough" },
            { AZ::Render::DisplayMapperOperationType::AcesLut, "AcesLut" },
            { AZ::Render::DisplayMapperOperationType::Aces, "Aces" } };

        for (auto operationNamePair : m_operationNames)
        {
            m_operationActions[operationNamePair.first] = toneMappingMenu->addAction(operationNamePair.second, [this, operationNamePair]() {
                MaterialCanvasViewportSettingsRequestBus::Event(
                    m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::SetDisplayMapperOperationType, operationNamePair.first);
            });
            m_operationActions[operationNamePair.first]->setCheckable(true);
        }

        toneMappingButton->setMenu(toneMappingMenu);
        toneMappingButton->setText("Tone Mapping");
        toneMappingButton->setIcon(QIcon(":/Icons/toneMapping.svg"));
        toneMappingButton->setPopupMode(QToolButton::InstantPopup);
        toneMappingButton->setVisible(true);
        addWidget(toneMappingButton);

        // Add lighting preset combo box
        m_lightingPresetComboBox = new AtomToolsFramework::AssetSelectionComboBox([](const AZ::Data::AssetInfo& assetInfo) {
            return assetInfo.m_assetType == AZ::RPI::AnyAsset::RTTI_Type() &&
                AZ::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), AZ::Render::LightingPreset::Extension);
        }, this);
        connect(m_lightingPresetComboBox, &AtomToolsFramework::AssetSelectionComboBox::AssetSelected, this, [this](const AZ::Data::AssetId& assetId) {
            MaterialCanvasViewportSettingsRequestBus::Event(
                m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::LoadLightingPresetByAssetId, assetId);
        });
        addWidget(m_lightingPresetComboBox);

        // Add model preset combo box
        m_modelPresetComboBox = new AtomToolsFramework::AssetSelectionComboBox([](const AZ::Data::AssetInfo& assetInfo) {
            return assetInfo.m_assetType == AZ::RPI::AnyAsset::RTTI_Type() &&
                AZ::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), AZ::Render::ModelPreset::Extension);
        }, this);
        connect(m_modelPresetComboBox, &AtomToolsFramework::AssetSelectionComboBox::AssetSelected, this, [this](const AZ::Data::AssetId& assetId) {
            MaterialCanvasViewportSettingsRequestBus::Event(
                m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::LoadModelPresetByAssetId, assetId);
        });
        addWidget(m_modelPresetComboBox);

        OnViewportSettingsChanged();

        MaterialCanvasViewportSettingsNotificationBus::Handler::BusConnect(m_toolId);
    }

    MaterialCanvasToolBar::~MaterialCanvasToolBar()
    {
        MaterialCanvasViewportSettingsNotificationBus::Handler::BusDisconnect();
    }

    void MaterialCanvasToolBar::OnViewportSettingsChanged()
    {
        MaterialCanvasViewportSettingsRequestBus::Event(
            m_toolId,
            [this](MaterialCanvasViewportRequests* viewportRequests)
            {
                m_toggleGrid->setChecked(viewportRequests->GetGridEnabled());
                m_toggleShadowCatcher->setChecked(viewportRequests->GetShadowCatcherEnabled());
                m_toggleAlternateSkybox->setChecked(viewportRequests->GetAlternateSkyboxEnabled());
                m_lightingPresetComboBox->SelectAsset(viewportRequests->GetLastLightingPresetAssetId());
                m_modelPresetComboBox->SelectAsset(viewportRequests->GetLastModelPresetAssetId());
                for (auto operationNamePair : m_operationNames)
                {
                    m_operationActions[operationNamePair.first]->setChecked(
                        operationNamePair.first == viewportRequests->GetDisplayMapperOperationType());
                }
            });
    }
} // namespace MaterialCanvas

#include <Window/ToolBar/moc_MaterialCanvasToolBar.cpp>
