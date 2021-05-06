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

#include <Atom/Document/MaterialEditorSettingsBus.h>
#include <Atom/Viewport/MaterialViewportNotificationBus.h>
#include <Atom/Viewport/MaterialViewportRequestBus.h>
#include <AzCore/std/containers/vector.h>
#include <Source/Window/ToolBar/LightingPresetComboBox.h>
#include <Source/Window/ToolBar/MaterialEditorToolBar.h>
#include <Source/Window/ToolBar/ModelPresetComboBox.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <AzQtComponents/Components/Widgets/ToolBar.h>
#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QToolButton>
AZ_POP_DISABLE_WARNING

namespace MaterialEditor
{
    MaterialEditorToolBar::MaterialEditorToolBar(QWidget* parent)
        : QToolBar(parent)
    {
        AzQtComponents::ToolBar::addMainToolBarStyle(this);

        // Add toggle grid button
        m_toggleGrid = addAction(QIcon(":/Icons/grid.svg"), "Toggle Grid");
        m_toggleGrid->setCheckable(true);
        connect(m_toggleGrid, &QAction::triggered, [this]() {
            MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::SetGridEnabled, m_toggleGrid->isChecked());
            });
        bool enableGrid = false;
        MaterialViewportRequestBus::BroadcastResult(enableGrid, &MaterialViewportRequestBus::Events::GetGridEnabled);
        m_toggleGrid->setChecked(enableGrid);

        // Add toggle shadow catcher button
        m_toggleShadowCatcher = addAction(QIcon(":/Icons/shadow.svg"), "Toggle Shadow Catcher");
        m_toggleShadowCatcher->setCheckable(true);
        connect(m_toggleShadowCatcher, &QAction::triggered, [this]() {
            MaterialViewportRequestBus::Broadcast(
                &MaterialViewportRequestBus::Events::SetShadowCatcherEnabled, m_toggleShadowCatcher->isChecked());
            });
        bool enableShadowCatcher = false;
        MaterialViewportRequestBus::BroadcastResult(enableShadowCatcher, &MaterialViewportRequestBus::Events::GetShadowCatcherEnabled);
        m_toggleShadowCatcher->setChecked(enableShadowCatcher);

        // Add mapping selection button
        
        QToolButton* toneMappingButton = new QToolButton(this);
        QMenu* toneMappingMenu = new QMenu(toneMappingButton);

        m_operationNames =
        {
            { AZ::Render::DisplayMapperOperationType::Reinhard, "Reinhard" },
            { AZ::Render::DisplayMapperOperationType::GammaSRGB, "GammaSRGB" },
            { AZ::Render::DisplayMapperOperationType::Passthrough, "Passthrough" },
            { AZ::Render::DisplayMapperOperationType::AcesLut, "AcesLut" },
            { AZ::Render::DisplayMapperOperationType::Aces, "Aces" }
        };
        for (auto operationNamePair : m_operationNames)
        {
            m_operationActions[operationNamePair.first] = toneMappingMenu->addAction(operationNamePair.second, [operationNamePair]() {
                MaterialViewportRequestBus::Broadcast(
                    &MaterialViewportRequestBus::Events::SetDisplayMapperOperationType,
                    operationNamePair.first);
                });
            m_operationActions[operationNamePair.first]->setCheckable(true);
        }
        m_operationActions[AZ::Render::DisplayMapperOperationType::Aces]->setChecked(true);
        toneMappingButton->setMenu(toneMappingMenu);
        toneMappingButton->setText("Tone Mapping");
        toneMappingButton->setIcon(QIcon(":/Icons/toneMapping.svg"));
        toneMappingButton->setPopupMode(QToolButton::InstantPopup);
        toneMappingButton->setVisible(true);
        addWidget(toneMappingButton);

        // Add model combo box
        auto modelPresetComboBox = new ModelPresetComboBox(this);
        modelPresetComboBox->setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy::AdjustToContents);
        addWidget(modelPresetComboBox);

        // Add lighting preset combo box
        auto lightingPresetComboBox = new LightingPresetComboBox(this);
        lightingPresetComboBox->setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy::AdjustToContents);
        addWidget(lightingPresetComboBox);

        MaterialViewportNotificationBus::Handler::BusConnect();
    }

    MaterialEditorToolBar::~MaterialEditorToolBar()
    {
        MaterialViewportNotificationBus::Handler::BusDisconnect();
    }

    void MaterialEditorToolBar::OnGridEnabledChanged(bool enable)
    {
        m_toggleGrid->setChecked(enable);
    }

    void MaterialEditorToolBar::OnDisplayMapperOperationTypeChanged(AZ::Render::DisplayMapperOperationType operationType)
    {
        for (auto operationActionPair : m_operationActions)
        {
            operationActionPair.second->setChecked(operationActionPair.first == operationType);
        }
    }

    void MaterialEditorToolBar::OnShadowCatcherEnabledChanged(bool enable)
    {
        m_toggleShadowCatcher->setChecked(enable);
    }

} // namespace MaterialEditor

#include <Source/Window/ToolBar/moc_MaterialEditorToolBar.cpp>
