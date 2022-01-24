/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/Feature/Utils/ModelPreset.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <AtomToolsFramework/AssetGridDialog/AssetGridDialog.h>
#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Application/Application.h>
#include <Viewport/MaterialViewportRequestBus.h>
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
            MaterialViewportRequestBus::Broadcast(
                [&savePath](MaterialViewportRequestBus::Events* viewportRequests)
                {
                    AZ::Render::ModelPresetPtr preset = viewportRequests->AddModelPreset(AZ::Render::ModelPreset());
                    viewportRequests->SaveModelPreset(preset, savePath);
                    viewportRequests->SelectModelPreset(preset);
                });
        }
    }

    void ViewportSettingsInspector::SelectModelPreset()
    {
        AZ::Data::AssetId selectedAsset;
        AtomToolsFramework::AssetGridDialog::SelectableAssetVector selectableAssets;
        AZStd::unordered_map<AZ::Data::AssetId, AZ::Render::ModelPresetPtr> assetIdToPresetMap;
        MaterialViewportRequestBus::Broadcast(
            [&](MaterialViewportRequestBus::Events* viewportRequests)
            {
                const auto& selectedPreset = viewportRequests->GetModelPresetSelection();
                selectedAsset = selectedPreset->m_modelAsset.GetId();

                const auto& presets = viewportRequests->GetModelPresets();
                selectableAssets.reserve(presets.size());
                for (const auto& preset : presets)
                {
                    const auto& presetAssetId = preset->m_modelAsset.GetId();
                    selectableAssets.push_back({ presetAssetId, preset->m_displayName.c_str() });
                    assetIdToPresetMap[presetAssetId] = preset;
                }
            });

        const int itemSize = aznumeric_cast<int>(
            AtomToolsFramework::GetSettingOrDefault<AZ::u64>("/O3DE/Atom/MaterialEditor/AssetGridDialog/ModelItemSize", 180));

        AtomToolsFramework::AssetGridDialog dialog(
            "Model Preset Browser", selectableAssets, selectedAsset, QSize(itemSize, itemSize), QApplication::activeWindow());

        connect(
            &dialog, &AtomToolsFramework::AssetGridDialog::AssetSelected, this,
            [assetIdToPresetMap](const AZ::Data::AssetId& assetId)
            {
                const auto presetItr = assetIdToPresetMap.find(assetId);
                if (presetItr != assetIdToPresetMap.end())
                {
                    MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::SelectModelPreset, presetItr->second);
                }
            });

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
            MaterialViewportRequestBus::Broadcast(
                [&savePath](MaterialViewportRequestBus::Events* viewportRequests)
                {
                    AZ::Render::LightingPresetPtr preset = viewportRequests->AddLightingPreset(AZ::Render::LightingPreset());
                    viewportRequests->SaveLightingPreset(preset, savePath);
                    viewportRequests->SelectLightingPreset(preset);
                });
        }
    }

    void ViewportSettingsInspector::SelectLightingPreset()
    {
        AZ::Data::AssetId selectedAsset;
        AtomToolsFramework::AssetGridDialog::SelectableAssetVector selectableAssets;
        AZStd::unordered_map<AZ::Data::AssetId, AZ::Render::LightingPresetPtr> assetIdToPresetMap;
        MaterialViewportRequestBus::Broadcast(
            [&](MaterialViewportRequestBus::Events* viewportRequests)
            {
                const auto& selectedPreset = viewportRequests->GetLightingPresetSelection();
                const auto& selectedPresetPath = viewportRequests->GetLightingPresetLastSavePath(selectedPreset);
                selectedAsset = AZ::RPI::AssetUtils::MakeAssetId(selectedPresetPath, 0).GetValue();

                const auto& presets = viewportRequests->GetLightingPresets();
                selectableAssets.reserve(presets.size());
                for (const auto& preset : presets)
                {
                    const auto& path = viewportRequests->GetLightingPresetLastSavePath(preset);
                    const auto& presetAssetId = AZ::RPI::AssetUtils::MakeAssetId(path, 0).GetValue();
                    selectableAssets.push_back({ presetAssetId, preset->m_displayName.c_str() });
                    assetIdToPresetMap[presetAssetId] = preset;
                }
            });

        const int itemSize = aznumeric_cast<int>(
            AtomToolsFramework::GetSettingOrDefault<AZ::u64>("/O3DE/Atom/MaterialEditor/AssetGridDialog/LightingItemSize", 180));

        AtomToolsFramework::AssetGridDialog dialog(
            "Lighting Preset Browser", selectableAssets, selectedAsset, QSize(itemSize, itemSize), QApplication::activeWindow());

        connect(
            &dialog, &AtomToolsFramework::AssetGridDialog::AssetSelected, this,
            [assetIdToPresetMap](const AZ::Data::AssetId& assetId)
            {
                const auto presetItr = assetIdToPresetMap.find(assetId);
                if (presetItr != assetIdToPresetMap.end())
                {
                    MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::SelectLightingPreset, presetItr->second);
                }
            });

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
        m_lightingPreset.reset();
        MaterialViewportRequestBus::Broadcast(
            [this](MaterialViewportRequestBus::Events* viewportRequests)
            {
                m_modelPreset = viewportRequests->GetModelPresetSelection();
                m_lightingPreset = viewportRequests->GetLightingPresetSelection();
                m_viewportSettings->m_enableGrid = viewportRequests->GetGridEnabled();
                m_viewportSettings->m_enableShadowCatcher = viewportRequests->GetShadowCatcherEnabled();
                m_viewportSettings->m_enableAlternateSkybox = viewportRequests->GetAlternateSkyboxEnabled();
                m_viewportSettings->m_fieldOfView = viewportRequests->GetFieldOfView();
                m_viewportSettings->m_displayMapperOperationType = viewportRequests->GetDisplayMapperOperationType();
            });

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

        MaterialViewportRequestBus::Broadcast(
            [this](MaterialViewportRequestBus::Events* viewportRequests)
            {
                viewportRequests->SetGridEnabled(m_viewportSettings->m_enableGrid);
                viewportRequests->SetShadowCatcherEnabled(m_viewportSettings->m_enableShadowCatcher);
                viewportRequests->SetAlternateSkyboxEnabled(m_viewportSettings->m_enableAlternateSkybox);
                viewportRequests->SetFieldOfView(m_viewportSettings->m_fieldOfView);
                viewportRequests->SetDisplayMapperOperationType(m_viewportSettings->m_displayMapperOperationType);
            });
    }

    AZStd::string ViewportSettingsInspector::GetDefaultUniqueSaveFilePath(const AZStd::string& baseName) const
    {
        return AtomToolsFramework::GetUniqueFileInfo(
            QString(AZ::Utils::GetProjectPath().c_str()) +
            AZ_CORRECT_FILESYSTEM_SEPARATOR + "Assets" +
            AZ_CORRECT_FILESYSTEM_SEPARATOR + baseName.c_str()).absoluteFilePath().toUtf8().constData();
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

#include <Window/ViewportSettingsInspector/moc_ViewportSettingsInspector.cpp>
