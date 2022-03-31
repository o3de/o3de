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
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AtomToolsFramework/AssetSelection/AssetSelectionGrid.h>
#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Application/Application.h>
#include <Viewport/MaterialCanvasViewportSettingsRequestBus.h>
#include <Window/ViewportSettingsInspector/ViewportSettingsInspector.h>

#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

namespace MaterialCanvas
{
    ViewportSettingsInspector::ViewportSettingsInspector(const AZ::Crc32& toolId, QWidget* parent)
        : AtomToolsFramework::InspectorWidget(parent)
        , m_toolId(toolId)
    {
        SetGroupSettingsPrefix("/O3DE/Atom/MaterialCanvas/ViewportSettingsInspector");
        Populate();
        MaterialCanvasViewportSettingsNotificationBus::Handler::BusConnect(m_toolId);
    }

    ViewportSettingsInspector::~ViewportSettingsInspector()
    {
        m_lightingPreset = {};
        m_modelPreset = {};
        MaterialCanvasViewportSettingsNotificationBus::Handler::BusDisconnect();
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
                &m_viewportSettings, &m_viewportSettings, m_viewportSettings.TYPEINFO_Uuid(), this, this, GetGroupSaveStateKey(groupName)));
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

        buttonGroupWidget->setLayout(new QHBoxLayout(buttonGroupWidget));
        buttonGroupWidget->layout()->addWidget(addButtonWidget);
        buttonGroupWidget->layout()->addWidget(selectButtonWidget);
        buttonGroupWidget->layout()->addWidget(saveButtonWidget);

        groupWidget->setLayout(new QVBoxLayout(groupWidget));
        groupWidget->layout()->addWidget(buttonGroupWidget);

        QObject::connect(addButtonWidget, &QPushButton::clicked, this, [this]() { CreateModelPreset(); });
        QObject::connect(selectButtonWidget, &QPushButton::clicked, this, [this]() { SelectModelPreset(); });
        QObject::connect(saveButtonWidget, &QPushButton::clicked, this, [this]() { SaveModelPreset(); });

        auto inspectorWidget = new AtomToolsFramework::InspectorPropertyGroupWidget(
            &m_modelPreset, &m_modelPreset, m_modelPreset.TYPEINFO_Uuid(), this, groupWidget, GetGroupSaveStateKey(groupName));

        groupWidget->layout()->addWidget(inspectorWidget);

        AddGroup(groupName, groupDisplayName, groupDescription, groupWidget);
    }

    void ViewportSettingsInspector::CreateModelPreset()
    {
        const AZStd::string defaultPath =
            AtomToolsFramework::GetUniqueDefaultSaveFilePath(AZStd::string::format("untited.%s", AZ::Render::ModelPreset::Extension));

        const AZStd::string savePath = AtomToolsFramework::GetSaveFilePath(defaultPath);
        if (!savePath.empty())
        {
            MaterialCanvasViewportSettingsRequestBus::Event(
                m_toolId,
                [&savePath](MaterialCanvasViewportSettingsRequestBus::Events* viewportRequests)
                {
                    viewportRequests->SetModelPreset(AZ::Render::ModelPreset());
                    viewportRequests->SaveModelPreset(savePath);
                });
        }
    }

    void ViewportSettingsInspector::SelectModelPreset()
    {
        const int itemSize = aznumeric_cast<int>(AtomToolsFramework::GetSettingsValue<AZ::u64>(
            "/O3DE/Atom/MaterialCanvas/ViewportSettingsInspector/AssetSelectionGrid/ModelItemSize", 128));

        AtomToolsFramework::AssetSelectionGrid dialog("Model Preset Browser", [](const AZ::Data::AssetInfo& assetInfo) {
            return assetInfo.m_assetType == AZ::RPI::AnyAsset::RTTI_Type() &&
                AZ::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), AZ::Render::ModelPreset::Extension);
        }, QSize(itemSize, itemSize), QApplication::activeWindow());

        AZ::Data::AssetId assetId;
        MaterialCanvasViewportSettingsRequestBus::EventResult(
            assetId, m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::GetLastModelPresetAssetId);
        dialog.SelectAsset(assetId);

        connect(&dialog, &AtomToolsFramework::AssetSelectionGrid::AssetRejected, this, [this, assetId]() {
            MaterialCanvasViewportSettingsRequestBus::Event(
                m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::LoadModelPresetByAssetId, assetId);
        });

        connect(&dialog, &AtomToolsFramework::AssetSelectionGrid::AssetSelected, this, [this](const AZ::Data::AssetId& assetId) {
            MaterialCanvasViewportSettingsRequestBus::Event(
                m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::LoadModelPresetByAssetId, assetId);
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
        AZStd::string defaultPath;
        MaterialCanvasViewportSettingsRequestBus::EventResult(
            defaultPath, m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::GetLastModelPresetPath);

        if (defaultPath.empty())
        {
            defaultPath =
                AtomToolsFramework::GetUniqueDefaultSaveFilePath(AZStd::string::format("untited.%s", AZ::Render::ModelPreset::Extension));
        }

        const AZStd::string savePath = AtomToolsFramework::GetSaveFilePath(defaultPath);
        if (!savePath.empty())
        {
            MaterialCanvasViewportSettingsRequestBus::Event(m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::SetModelPreset, m_modelPreset);
            MaterialCanvasViewportSettingsRequestBus::Event(m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::SaveModelPreset, savePath);
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

        buttonGroupWidget->setLayout(new QHBoxLayout(buttonGroupWidget));
        buttonGroupWidget->layout()->addWidget(addButtonWidget);
        buttonGroupWidget->layout()->addWidget(selectButtonWidget);
        buttonGroupWidget->layout()->addWidget(saveButtonWidget);

        groupWidget->setLayout(new QVBoxLayout(groupWidget));
        groupWidget->layout()->addWidget(buttonGroupWidget);

        QObject::connect(addButtonWidget, &QPushButton::clicked, this, [this]() { CreateLightingPreset(); });
        QObject::connect(selectButtonWidget, &QPushButton::clicked, this, [this]() { SelectLightingPreset(); });
        QObject::connect(saveButtonWidget, &QPushButton::clicked, this, [this]() { SaveLightingPreset(); });

        auto inspectorWidget = new AtomToolsFramework::InspectorPropertyGroupWidget(
            &m_lightingPreset, &m_lightingPreset, m_lightingPreset.TYPEINFO_Uuid(), this, groupWidget, GetGroupSaveStateKey(groupName));

        groupWidget->layout()->addWidget(inspectorWidget);

        AddGroup(groupName, groupDisplayName, groupDescription, groupWidget);
    }

    void ViewportSettingsInspector::CreateLightingPreset()
    {
        const AZStd::string defaultPath =
            AtomToolsFramework::GetUniqueDefaultSaveFilePath(AZStd::string::format("untited.%s", AZ::Render::LightingPreset::Extension));

        const AZStd::string savePath = AtomToolsFramework::GetSaveFilePath(defaultPath);
        if (!savePath.empty())
        {
            MaterialCanvasViewportSettingsRequestBus::Event(
                m_toolId,
                [&savePath](MaterialCanvasViewportSettingsRequestBus::Events* viewportRequests)
                {
                    viewportRequests->SetLightingPreset(AZ::Render::LightingPreset());
                    viewportRequests->SaveLightingPreset(savePath);
                });
        }
    }

    void ViewportSettingsInspector::SelectLightingPreset()
    {
        const int itemSize = aznumeric_cast<int>(AtomToolsFramework::GetSettingsValue<AZ::u64>(
            "/O3DE/Atom/MaterialCanvas/ViewportSettingsInspector/AssetSelectionGrid/LightingItemSize", 128));

        AtomToolsFramework::AssetSelectionGrid dialog("Lighting Preset Browser", [](const AZ::Data::AssetInfo& assetInfo) {
            return assetInfo.m_assetType == AZ::RPI::AnyAsset::RTTI_Type() &&
                AZ::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), AZ::Render::LightingPreset::Extension);
        }, QSize(itemSize, itemSize), QApplication::activeWindow());

        AZ::Data::AssetId assetId;
        MaterialCanvasViewportSettingsRequestBus::EventResult(
            assetId, m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::GetLastLightingPresetAssetId);
        dialog.SelectAsset(assetId);

        connect(&dialog, &AtomToolsFramework::AssetSelectionGrid::AssetRejected, this, [this, assetId]() {
            MaterialCanvasViewportSettingsRequestBus::Event(
                m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::LoadLightingPresetByAssetId, assetId);
        });

        connect(&dialog, &AtomToolsFramework::AssetSelectionGrid::AssetSelected, this, [this](const AZ::Data::AssetId& assetId) {
            MaterialCanvasViewportSettingsRequestBus::Event(
                m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::LoadLightingPresetByAssetId, assetId);
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
        AZStd::string defaultPath;
        MaterialCanvasViewportSettingsRequestBus::EventResult(
            defaultPath, m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::GetLastLightingPresetPath);

        if (defaultPath.empty())
        {
            defaultPath = AtomToolsFramework::GetUniqueDefaultSaveFilePath(
                AZStd::string::format("untited.%s", AZ::Render::LightingPreset::Extension));
        }

        const AZStd::string savePath = AtomToolsFramework::GetSaveFilePath(defaultPath);
        if (!savePath.empty())
        {
            MaterialCanvasViewportSettingsRequestBus::Event(m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::SetLightingPreset, m_lightingPreset);
            MaterialCanvasViewportSettingsRequestBus::Event(m_toolId, &MaterialCanvasViewportSettingsRequestBus::Events::SaveLightingPreset, savePath);
        }
    }

    void ViewportSettingsInspector::SaveSettings()
    {
        MaterialCanvasViewportSettingsRequestBus::Event(
            m_toolId,
            [this](MaterialCanvasViewportSettingsRequestBus::Events* viewportRequests)
            {
                viewportRequests->SetModelPreset(m_modelPreset);
                viewportRequests->SetLightingPreset(m_lightingPreset);
                viewportRequests->SetGridEnabled(m_viewportSettings.m_enableGrid);
                viewportRequests->SetShadowCatcherEnabled(m_viewportSettings.m_enableShadowCatcher);
                viewportRequests->SetAlternateSkyboxEnabled(m_viewportSettings.m_enableAlternateSkybox);
                viewportRequests->SetFieldOfView(m_viewportSettings.m_fieldOfView);
                viewportRequests->SetDisplayMapperOperationType(m_viewportSettings.m_displayMapperOperationType);
            });
    }

    void ViewportSettingsInspector::LoadSettings()
    {
        MaterialCanvasViewportSettingsRequestBus::Event(
            m_toolId,
            [this](MaterialCanvasViewportSettingsRequestBus::Events* viewportRequests)
            {
                m_modelPreset = viewportRequests->GetModelPreset();
                m_lightingPreset = viewportRequests->GetLightingPreset();
                m_viewportSettings.m_enableGrid = viewportRequests->GetGridEnabled();
                m_viewportSettings.m_enableShadowCatcher = viewportRequests->GetShadowCatcherEnabled();
                m_viewportSettings.m_enableAlternateSkybox = viewportRequests->GetAlternateSkyboxEnabled();
                m_viewportSettings.m_fieldOfView = viewportRequests->GetFieldOfView();
                m_viewportSettings.m_displayMapperOperationType = viewportRequests->GetDisplayMapperOperationType();
            });
    }

    void ViewportSettingsInspector::Reset()
    {
        LoadSettings();
        AtomToolsFramework::InspectorWidget::Reset();
    }

    void ViewportSettingsInspector::OnViewportSettingsChanged()
    {
        LoadSettings();
        RefreshAll();
    }

    void ViewportSettingsInspector::AfterPropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode)
    {
        SaveSettings();
    }

    void ViewportSettingsInspector::SetPropertyEditingComplete([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode)
    {
        SaveSettings();
    }

    AZ::Crc32 ViewportSettingsInspector::GetGroupSaveStateKey(const AZStd::string& groupName) const
    {
        return AZ::Crc32(AZStd::string::format("/O3DE/Atom/MaterialCanvas/ViewportSettingsInspector/PropertyGroup/%s", groupName.c_str()));
    }
} // namespace MaterialCanvas

#include <Window/ViewportSettingsInspector/moc_ViewportSettingsInspector.cpp>
