/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Viewport/MaterialViewportRequestBus.h>
#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <AtomToolsFramework/Util/Util.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Window/PresetBrowserDialogs/LightingPresetBrowserDialog.h>
#include <Window/PresetBrowserDialogs/ModelPresetBrowserDialog.h>
#include <Window/ViewportSettingsInspector/ViewportSettingsInspector.h>

#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

namespace MaterialEditor
{
    ViewportSettingsInspector::ViewportSettingsInspector(QWidget* parent)
        : AtomToolsFramework::InspectorWidget(parent)
    {
        m_viewportSettings =
            AZ::UserSettings::CreateFind<MaterialViewportSettings>(AZ::Crc32("MaterialViewportSettings"), AZ::UserSettings::CT_GLOBAL);

        m_windowSettings = AZ::UserSettings::CreateFind<MaterialEditorWindowSettings>(
            AZ::Crc32("MaterialEditorWindowSettings"), AZ::UserSettings::CT_GLOBAL);

        MaterialViewportNotificationBus::Handler::BusConnect();
    }

    ViewportSettingsInspector::~ViewportSettingsInspector()
    {
        m_lightingPreset.reset();
        m_modelPreset.reset();
        MaterialViewportNotificationBus::Handler::BusDisconnect();
        AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
    }

    void ViewportSettingsInspector::Populate()
    {
        AddGroupsBegin();
        AddGeneralGroup();
        AddModelGroup();
        AddLightingGroup();
        AddGroupsEnd();
    }

    void ViewportSettingsInspector::AddGeneralGroup()
    {
        const AZStd::string groupName = "generalSettings";
        const AZStd::string groupDisplayName = "General Settings";
        const AZStd::string groupDescription = "General Settings";

        AddGroup(
            groupName, groupDisplayName, groupDescription,
            new AtomToolsFramework::InspectorPropertyGroupWidget(
                m_viewportSettings.get(), nullptr, m_viewportSettings->TYPEINFO_Uuid(), this, this, GetGroupSaveStateKey(groupName)));
    }

    void ViewportSettingsInspector::AddModelGroup()
    {
        const AZStd::string groupName = "modelSettings";
        const AZStd::string groupDisplayName = "Model Settings";
        const AZStd::string groupDescription = "Model Settings";

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
            auto inspectorWidget = new AtomToolsFramework::InspectorPropertyGroupWidget(
                m_modelPreset.get(), nullptr, m_modelPreset.get()->TYPEINFO_Uuid(), this, groupWidget, GetGroupSaveStateKey(groupName));

            groupWidget->layout()->addWidget(inspectorWidget);
        }

        AddGroup(groupName, groupDisplayName, groupDescription, groupWidget);
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
        const AZStd::string groupName = "lightingSettings";
        const AZStd::string groupDisplayName = "Lighting Settings";
        const AZStd::string groupDescription = "Lighting Settings";

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
            auto inspectorWidget = new AtomToolsFramework::InspectorPropertyGroupWidget(
                m_lightingPreset.get(), nullptr, m_lightingPreset.get()->TYPEINFO_Uuid(), this, groupWidget,
                GetGroupSaveStateKey(groupName));

            groupWidget->layout()->addWidget(inspectorWidget);
        }

        AddGroup(groupName, groupDisplayName, groupDescription, groupWidget);
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
            MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::SaveLightingPreset, preset, savePath);
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
        MaterialViewportRequestBus::BroadcastResult(
            defaultPath, &MaterialViewportRequestBus::Events::GetLightingPresetLastSavePath, preset);

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
        MaterialViewportRequestBus::BroadcastResult(
            m_viewportSettings->m_fieldOfView, &MaterialViewportRequestBus::Handler::GetFieldOfView);
        MaterialViewportRequestBus::BroadcastResult(
            m_viewportSettings->m_displayMapperOperationType, &MaterialViewportRequestBus::Handler::GetDisplayMapperOperationType);

        AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
        AtomToolsFramework::InspectorWidget::Reset();
    }

    void ViewportSettingsInspector::OnLightingPresetSelected([[maybe_unused]] AZ::Render::LightingPresetPtr preset)
    {
        if (m_lightingPreset != preset)
        {
            Populate();
        }
    }

    void ViewportSettingsInspector::OnModelPresetSelected([[maybe_unused]] AZ::Render::ModelPresetPtr preset)
    {
        if (m_modelPreset != preset)
        {
            Populate();
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
        MaterialViewportRequestBus::Broadcast(
            &MaterialViewportRequestBus::Handler::SetDisplayMapperOperationType, m_viewportSettings->m_displayMapperOperationType);
    }

    AZStd::string ViewportSettingsInspector::GetDefaultUniqueSaveFilePath(const AZStd::string& baseName) const
    {
        AZStd::string savePath = AZ::IO::FileIOBase::GetInstance()->GetAlias("@projectroot@");
        savePath += AZ_CORRECT_FILESYSTEM_SEPARATOR;
        savePath += "Materials";
        savePath += AZ_CORRECT_FILESYSTEM_SEPARATOR;
        savePath += baseName;
        savePath = AtomToolsFramework::GetUniqueFileInfo(savePath.c_str()).absoluteFilePath().toUtf8().constData();
        return savePath;
    }

    AZ::Crc32 ViewportSettingsInspector::GetGroupSaveStateKey(const AZStd::string& groupName) const
    {
        return AZ::Crc32(AZStd::string::format("ViewportSettingsInspector::PropertyGroup::%s", groupName.c_str()));
    }

    bool ViewportSettingsInspector::ShouldGroupAutoExpanded(const AZStd::string& groupName) const
    {
        auto stateItr = m_windowSettings->m_inspectorCollapsedGroups.find(GetGroupSaveStateKey(groupName));
        return stateItr == m_windowSettings->m_inspectorCollapsedGroups.end();
    }

    void ViewportSettingsInspector::OnGroupExpanded(const AZStd::string& groupName)
    {
        m_windowSettings->m_inspectorCollapsedGroups.erase(GetGroupSaveStateKey(groupName));
    }

    void ViewportSettingsInspector::OnGroupCollapsed(const AZStd::string& groupName)
    {
        m_windowSettings->m_inspectorCollapsedGroups.insert(GetGroupSaveStateKey(groupName));
    }

} // namespace MaterialEditor

#include <Source/Window/ViewportSettingsInspector/moc_ViewportSettingsInspector.cpp>
