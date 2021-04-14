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
        //[GFX TODO][ATOM-3992]
        QToolButton* toneMappingButton = new QToolButton(this);
        QMenu* toneMappingMenu = new QMenu(toneMappingButton);
        toneMappingMenu->addAction("None", [this]() {
            MaterialEditorSettingsRequestBus::Broadcast(&MaterialEditorSettingsRequests::SetStringProperty, "toneMapping", "None");
        });
        toneMappingMenu->addAction("Gamma2.2", [this]() {
            MaterialEditorSettingsRequestBus::Broadcast(&MaterialEditorSettingsRequests::SetStringProperty, "toneMapping", "Gamma2.2");
        });
        toneMappingMenu->addAction("ACES", [this]() {
            MaterialEditorSettingsRequestBus::Broadcast(&MaterialEditorSettingsRequests::SetStringProperty, "toneMapping", "ACES");
        });
        toneMappingButton->setMenu(toneMappingMenu);
        toneMappingButton->setText("Tone Mapping");
        toneMappingButton->setIcon(QIcon(":/Icons/toneMapping.svg"));
        toneMappingButton->setPopupMode(QToolButton::InstantPopup);

        // hiding button until DisplayMapper supports changing settings at run time
        toneMappingButton->setVisible(false);
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

    void MaterialEditorToolBar::OnShadowCatcherEnabledChanged(bool enable)
    {
        m_toggleShadowCatcher->setChecked(enable);
    }

} // namespace MaterialEditor

#include <Source/Window/ToolBar/moc_MaterialEditorToolBar.cpp>
