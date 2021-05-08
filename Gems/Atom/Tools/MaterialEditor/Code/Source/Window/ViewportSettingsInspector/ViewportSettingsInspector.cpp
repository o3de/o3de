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

#include <Window/ViewportSettingsInspector/ViewportSettingsInspector.h>
#include <Atom/Viewport/MaterialViewportRequestBus.h>
#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <AtomToolsFramework/Util/Util.h>
#include <Window/PresetBrowserDialogs/LightingPresetBrowserDialog.h>
#include <Window/PresetBrowserDialogs/ModelPresetBrowserDialog.h>

#include <QApplication>
#include <QPushButton>
#include <QToolButton>
#include <QAction>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace MaterialEditor
{
    ViewportSettingsInspector::ViewportSettingsInspector(QWidget* parent)
        : AtomToolsFramework::InspectorWidget(parent)
    {
        m_viewportSettings =
            AZ::UserSettings::CreateFind<MaterialViewportSettings>(AZ::Crc32("MaterialViewportSettings"), AZ::UserSettings::CT_GLOBAL);

        MaterialViewportNotificationBus::Handler::BusConnect();
    }

    ViewportSettingsInspector::~ViewportSettingsInspector()
    {
        m_lightingPreset.reset();
        m_modelPreset.reset();
        MaterialViewportNotificationBus::Handler::BusDisconnect();
        AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
    }

    void ViewportSettingsInspector::Popuate()
    {
        AddGroupsBegin();
        AddGeneralGroup();
        AddModelGroup();
        AddLightingGroup();
        AddGroupsEnd();
    }

    void ViewportSettingsInspector::AddGeneralGroup()
    {
        const AZStd::string groupNameId = "general";
        const AZStd::string groupDisplayName = "General";
        const AZStd::string groupDescription = "General";

        const AZ::Crc32 saveStateKey(AZStd::string::format("ViewportSettingsInspector::GeneralGroup"));
        AddGroup(
            groupNameId, groupDisplayName, groupDescription,
            new AtomToolsFramework::InspectorPropertyGroupWidget(
                m_viewportSettings.get(), nullptr, m_viewportSettings->TYPEINFO_Uuid(), this, this, saveStateKey));
    }

    void ViewportSettingsInspector::AddModelGroup()
    {
        const AZStd::string groupNameId = "model";
        const AZStd::string groupDisplayName = "Model";
        const AZStd::string groupDescription = "Model";

        auto groupWidget = new QWidget(this);
        auto buttonGroupWidget = new QWidget(groupWidget);
        auto addButtonWidget = new QPushButton("Add", buttonGroupWidget);
        auto selectButtonWidget = new QPushButton("Select", buttonGroupWidget);
        auto saveButtonWidget = new QPushButton("Save", buttonGroupWidget);
        auto refreshButtonWidget = new QPushButton("Refresh", buttonGroupWidget);

        buttonGroupWidget->setLayout(new QHBoxLayout(buttonGroupWidget));
        buttonGroupWidget->layout()->addWidget(addButtonWidget);
        buttonGroupWidget->layout()->addWidget(selectButtonWidget);
        buttonGroupWidget->layout()->addWidget(saveButtonWidget);
        buttonGroupWidget->layout()->addWidget(refreshButtonWidget);

        groupWidget->setLayout(new QVBoxLayout(groupWidget));
        groupWidget->layout()->addWidget(buttonGroupWidget);

        QObject::connect(addButtonWidget, &QPushButton::clicked, this, [this]() { AddModelPreset(); });
        QObject::connect(selectButtonWidget, &QPushButton::clicked, this, [this]() { SelectModelPreset(); });
        QObject::connect(saveButtonWidget, &QPushButton::clicked, this, [this]() { SaveModelPreset(); });
        QObject::connect(refreshButtonWidget, &QPushButton::clicked, this, [this]() { RefreshPresets(); });

        if (m_modelPreset)
        {
            const AZ::Crc32 saveStateKey(AZStd::string::format("ViewportSettingsInspector::ModelGroup"));
            auto inspectorWidget = new AtomToolsFramework::InspectorPropertyGroupWidget(
                m_modelPreset.get(), nullptr, m_modelPreset.get()->TYPEINFO_Uuid(), this, groupWidget, saveStateKey);

            groupWidget->layout()->addWidget(inspectorWidget);
        }

        AddGroup(groupNameId, groupDisplayName, groupDescription, groupWidget);
    }

    void ViewportSettingsInspector::AddModelPreset()
    {
        const AZStd::string defaultPath = GetDefaultUniqueSaveFilePath("untitled.modelpreset.azasset");
        const AZStd::string savePath = AtomToolsFramework::GetSaveFileInfo(defaultPath.c_str()).absoluteFilePath().toUtf8().constData();

        if (!savePath.empty())
        {
            AZ::Render::ModelPresetPtr preset;
            MaterialViewportRequestBus::BroadcastResult(
                preset, &MaterialViewportRequestBus::Events::AddModelPreset, AZ::Render::ModelPreset());
            MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::SaveModelPreset, preset, savePath);
            MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::SelectModelPreset, preset);
        }
    }

    void ViewportSettingsInspector::SelectModelPreset()
    {
        ModelPresetBrowserDialog dialog(QApplication::activeWindow());

        dialog.setFixedSize(800, 400);
        dialog.show();

        // Removing fixed size to allow drag resizing
        dialog.setMinimumSize(0, 0);
        dialog.setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        dialog.exec();
    }

    void ViewportSettingsInspector::SaveModelPreset()
    {
        AZ::Render::ModelPresetPtr preset;
        MaterialViewportRequestBus::BroadcastResult(preset, &MaterialViewportRequestBus::Events::GetModelPresetSelection);

        AZStd::string defaultPath;
        MaterialViewportRequestBus::BroadcastResult(defaultPath, &MaterialViewportRequestBus::Events::GetModelPresetLastSavePath, preset);

        if (defaultPath.empty())
        {
            defaultPath = GetDefaultUniqueSaveFilePath("untitled.modelpreset.azasset");
        }

        const AZStd::string savePath = AtomToolsFramework::GetSaveFileInfo(defaultPath.c_str()).absoluteFilePath().toUtf8().constData();

        if (!savePath.empty())
        {
            MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::SaveModelPreset, preset, savePath);
        }
    }

    void ViewportSettingsInspector::AddLightingGroup()
    {
        const AZStd::string groupNameId = "lighting";
        const AZStd::string groupDisplayName = "Lighting";
        const AZStd::string groupDescription = "Lighting";

        auto groupWidget = new QWidget(this);
        auto buttonGroupWidget = new QWidget(groupWidget);
        auto addButtonWidget = new QPushButton("Add", buttonGroupWidget);
        auto selectButtonWidget = new QPushButton("Select", buttonGroupWidget);
        auto saveButtonWidget = new QPushButton("Save", buttonGroupWidget);
        auto refreshButtonWidget = new QPushButton("Refresh", buttonGroupWidget);

        buttonGroupWidget->setLayout(new QHBoxLayout(buttonGroupWidget));
        buttonGroupWidget->layout()->addWidget(addButtonWidget);
        buttonGroupWidget->layout()->addWidget(selectButtonWidget);
        buttonGroupWidget->layout()->addWidget(saveButtonWidget);
        buttonGroupWidget->layout()->addWidget(refreshButtonWidget);

        groupWidget->setLayout(new QVBoxLayout(groupWidget));
        groupWidget->layout()->addWidget(buttonGroupWidget);

        QObject::connect(addButtonWidget, &QPushButton::clicked, this, [this]() { AddLightingPreset(); });
        QObject::connect(selectButtonWidget, &QPushButton::clicked, this, [this]() { SelectLightingPreset(); });
        QObject::connect(saveButtonWidget, &QPushButton::clicked, this, [this]() { SaveLightingPreset(); });
        QObject::connect(refreshButtonWidget, &QPushButton::clicked, this, [this]() { RefreshPresets(); });

        if (m_lightingPreset)
        {
            const AZ::Crc32 saveStateKey(AZStd::string::format("ViewportSettingsInspector::LightingGroup"));
            auto inspectorWidget = new AtomToolsFramework::InspectorPropertyGroupWidget(
                m_lightingPreset.get(), nullptr, m_lightingPreset.get()->TYPEINFO_Uuid(), this, groupWidget, saveStateKey);

            groupWidget->layout()->addWidget(inspectorWidget);
        }

        AddGroup(groupNameId, groupDisplayName, groupDescription, groupWidget);
    }

    void ViewportSettingsInspector::AddLightingPreset()
    {
        const AZStd::string defaultPath = GetDefaultUniqueSaveFilePath("untitled.lightingpreset.azasset");
        const AZStd::string savePath = AtomToolsFramework::GetSaveFileInfo(defaultPath.c_str()).absoluteFilePath().toUtf8().constData();

        if (!savePath.empty())
        {
            AZ::Render::LightingPresetPtr preset;
            MaterialViewportRequestBus::BroadcastResult(
                preset, &MaterialViewportRequestBus::Events::AddLightingPreset, AZ::Render::LightingPreset());
            MaterialViewportRequestBus::Broadcast(
                &MaterialViewportRequestBus::Events::SaveLightingPreset, preset, savePath);
            MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::SelectLightingPreset, preset);
        }
    }

    void ViewportSettingsInspector::SelectLightingPreset()
    {
        LightingPresetBrowserDialog dialog(QApplication::activeWindow());

        dialog.setFixedSize(800, 400);
        dialog.show();

        // Removing fixed size to allow drag resizing
        dialog.setMinimumSize(0, 0);
        dialog.setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        dialog.exec();
    }

    void ViewportSettingsInspector::SaveLightingPreset()
    {
        AZ::Render::LightingPresetPtr preset;
        MaterialViewportRequestBus::BroadcastResult(preset, &MaterialViewportRequestBus::Events::GetLightingPresetSelection);

        AZStd::string defaultPath;
        MaterialViewportRequestBus::BroadcastResult(defaultPath, &MaterialViewportRequestBus::Events::GetLightingPresetLastSavePath, preset);

        if (defaultPath.empty())
        {
            defaultPath = GetDefaultUniqueSaveFilePath("untitled.lightingpreset.azasset");
        }

        const AZStd::string savePath = AtomToolsFramework::GetSaveFileInfo(defaultPath.c_str()).absoluteFilePath().toUtf8().constData();

        if (!savePath.empty())
        {
            MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::SaveLightingPreset, preset, savePath);
        }
    }

    void ViewportSettingsInspector::RefreshPresets()
    {
        MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::ReloadContent);
    }

    void ViewportSettingsInspector::Reset()
    {
        m_modelPreset.reset();
        MaterialViewportRequestBus::BroadcastResult(m_modelPreset, &MaterialViewportRequestBus::Events::GetModelPresetSelection);

        m_lightingPreset.reset();
        MaterialViewportRequestBus::BroadcastResult(m_lightingPreset, &MaterialViewportRequestBus::Events::GetLightingPresetSelection);

        MaterialViewportRequestBus::BroadcastResult(m_viewportSettings->m_enableGrid, &MaterialViewportRequestBus::Events::GetGridEnabled);
        MaterialViewportRequestBus::BroadcastResult(
            m_viewportSettings->m_enableShadowCatcher, &MaterialViewportRequestBus::Events::GetShadowCatcherEnabled);
        MaterialViewportRequestBus::BroadcastResult(
            m_viewportSettings->m_enableAlternateSkybox, &MaterialViewportRequestBus::Events::GetAlternateSkyboxEnabled);
        MaterialViewportRequestBus::BroadcastResult(m_viewportSettings->m_fieldOfView, &MaterialViewportRequestBus::Handler::GetFieldOfView);
        MaterialViewportRequestBus::BroadcastResult(m_viewportSettings->m_displayMapperOperationType, &MaterialViewportRequestBus::Handler::GetDisplayMapperOperationType);

        AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
        AtomToolsFramework::InspectorWidget::Reset();
    }

    void ViewportSettingsInspector::OnLightingPresetSelected([[maybe_unused]] AZ::Render::LightingPresetPtr preset)
    {
        if (m_lightingPreset != preset)
        {
            Popuate();
        }
    }

    void ViewportSettingsInspector::OnModelPresetSelected([[maybe_unused]] AZ::Render::ModelPresetPtr preset)
    {
        if (m_modelPreset != preset)
        {
            Popuate();
        }
    }

    void ViewportSettingsInspector::OnShadowCatcherEnabledChanged(bool enable)
    {
        m_viewportSettings->m_enableShadowCatcher = enable;
        RefreshGroup("general");
    }

    void ViewportSettingsInspector::OnGridEnabledChanged(bool enable)
    {
        m_viewportSettings->m_enableGrid = enable;
        RefreshGroup("general");
    }

    void ViewportSettingsInspector::OnAlternateSkyboxEnabledChanged(bool enable)
    {
        m_viewportSettings->m_enableAlternateSkybox = enable;
        RefreshGroup("general");
    }

    void ViewportSettingsInspector::OnFieldOfViewChanged(float fieldOfView)
    {
        m_viewportSettings->m_fieldOfView = fieldOfView;
        RefreshGroup("general");
    }

    void ViewportSettingsInspector::OnDisplayMapperOperationTypeChanged(AZ::Render::DisplayMapperOperationType operationType)
    {
        m_viewportSettings->m_displayMapperOperationType = operationType;
        RefreshGroup("general");
    }

    void ViewportSettingsInspector::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
    {
        AZ_UNUSED(pNode);
    }

    void ViewportSettingsInspector::AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode)
    {
        AZ_UNUSED(pNode);
        ApplyChanges();
    }

    void ViewportSettingsInspector::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
    {
        AZ_UNUSED(pNode);
        ApplyChanges();
    }

    void ViewportSettingsInspector::ApplyChanges()
    {
        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnLightingPresetChanged, m_lightingPreset);
        MaterialViewportNotificationBus::Broadcast(&MaterialViewportNotificationBus::Events::OnModelPresetChanged, m_modelPreset);
        MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::SetGridEnabled, m_viewportSettings->m_enableGrid);
        MaterialViewportRequestBus::Broadcast(
            &MaterialViewportRequestBus::Events::SetShadowCatcherEnabled, m_viewportSettings->m_enableShadowCatcher);
        MaterialViewportRequestBus::Broadcast(
            &MaterialViewportRequestBus::Events::SetAlternateSkyboxEnabled, m_viewportSettings->m_enableAlternateSkybox);
        MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Handler::SetFieldOfView, m_viewportSettings->m_fieldOfView);
        MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Handler::SetDisplayMapperOperationType, m_viewportSettings->m_displayMapperOperationType);
    }

    AZStd::string ViewportSettingsInspector::GetDefaultUniqueSaveFilePath(const AZStd::string& baseName) const
    {
        AZStd::string savePath = AZ::IO::FileIOBase::GetInstance()->GetAlias("@devassets@");
        savePath += AZ_CORRECT_FILESYSTEM_SEPARATOR;
        savePath += "Materials";
        savePath += AZ_CORRECT_FILESYSTEM_SEPARATOR;
        savePath += baseName;
        savePath = AtomToolsFramework::GetUniqueFileInfo(savePath.c_str()).absoluteFilePath().toUtf8().constData();
        return savePath;
    }
} // namespace MaterialEditor

#include <Source/Window/ViewportSettingsInspector/moc_ViewportSettingsInspector.cpp>
