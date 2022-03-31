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
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsInspector.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsRequestBus.h>
#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Application/Application.h>

#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

namespace AtomToolsFramework
{
    EntityPreviewViewportSettingsInspector::EntityPreviewViewportSettingsInspector(const AZ::Crc32& toolId, QWidget* parent)
        : InspectorWidget(parent)
        , m_toolId(toolId)
    {
        setObjectName("EntityPreviewViewportSettingsInspector");
        SetGroupSettingsPrefix("/O3DE/AtomToolsFramework/EntityPreviewViewportSettingsInspector");
        Populate();
        EntityPreviewViewportSettingsNotificationBus::Handler::BusConnect(m_toolId);
    }

    EntityPreviewViewportSettingsInspector::~EntityPreviewViewportSettingsInspector()
    {
        m_lightingPreset = {};
        m_modelPreset = {};
        EntityPreviewViewportSettingsNotificationBus::Handler::BusDisconnect();
    }

    void EntityPreviewViewportSettingsInspector::Populate()
    {
        AddGroupsBegin();
        AddGeneralGroup();
        AddModelGroup();
        AddLightingGroup();
        AddGroupsEnd();
    }

    void EntityPreviewViewportSettingsInspector::AddGeneralGroup()
    {
        const AZStd::string groupName = "generalSettings";
        const AZStd::string groupDisplayName = "General Settings";
        const AZStd::string groupDescription = "General Settings";

        AddGroup(
            groupName, groupDisplayName, groupDescription,
            new InspectorPropertyGroupWidget(
                &m_viewportSettings, &m_viewportSettings, m_viewportSettings.TYPEINFO_Uuid(), this, this, GetGroupSaveStateKey(groupName)));
    }

    void EntityPreviewViewportSettingsInspector::AddModelGroup()
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

        auto inspectorWidget = new InspectorPropertyGroupWidget(
            &m_modelPreset, &m_modelPreset, m_modelPreset.TYPEINFO_Uuid(), this, groupWidget, GetGroupSaveStateKey(groupName));

        groupWidget->layout()->addWidget(inspectorWidget);

        AddGroup(groupName, groupDisplayName, groupDescription, groupWidget);
    }

    void EntityPreviewViewportSettingsInspector::CreateModelPreset()
    {
        const AZStd::string defaultPath = GetUniqueDefaultSaveFilePath(AZ::Render::ModelPreset::Extension);
        const AZStd::string savePath = GetSaveFilePath(defaultPath);
        if (!savePath.empty())
        {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId,
                [&savePath](EntityPreviewViewportSettingsRequestBus::Events* viewportRequests)
                {
                    viewportRequests->SetModelPreset(AZ::Render::ModelPreset());
                    viewportRequests->SaveModelPreset(savePath);
                });
        }
    }

    void EntityPreviewViewportSettingsInspector::SelectModelPreset()
    {
        const int itemSize = aznumeric_cast<int>(GetSettingsValue<AZ::u64>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettingsInspector/AssetSelectionGrid/ModelItemSize", 128));

        AssetSelectionGrid dialog("Model Preset Browser", [](const AZ::Data::AssetInfo& assetInfo) {
            return assetInfo.m_assetType == AZ::RPI::AnyAsset::RTTI_Type() &&
                AZ::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), AZ::Render::ModelPreset::Extension);
        }, QSize(itemSize, itemSize), GetToolMainWindow());

        AZ::Data::AssetId assetId;
        EntityPreviewViewportSettingsRequestBus::EventResult(
            assetId, m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::GetLastModelPresetAssetId);
        dialog.SelectAsset(assetId);

        connect(&dialog, &AssetSelectionGrid::AssetRejected, this, [this, assetId]() {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadModelPresetByAssetId, assetId);
        });

        connect(&dialog, &AssetSelectionGrid::AssetSelected, this, [this](const AZ::Data::AssetId& assetId) {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadModelPresetByAssetId, assetId);
        });

        dialog.setFixedSize(800, 400);
        dialog.show();

        // Removing fixed size to allow drag resizing
        dialog.setMinimumSize(0, 0);
        dialog.setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        dialog.exec();
    }

    void EntityPreviewViewportSettingsInspector::SaveModelPreset()
    {
        AZStd::string defaultPath;
        EntityPreviewViewportSettingsRequestBus::EventResult(
            defaultPath, m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::GetLastModelPresetPath);

        if (defaultPath.empty())
        {
            defaultPath = GetUniqueDefaultSaveFilePath(AZ::Render::ModelPreset::Extension);
        }

        const AZStd::string savePath = GetSaveFilePath(defaultPath);
        if (!savePath.empty())
        {
            EntityPreviewViewportSettingsRequestBus::Event(m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SetModelPreset, m_modelPreset);
            EntityPreviewViewportSettingsRequestBus::Event(m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SaveModelPreset, savePath);
        }
    }

    void EntityPreviewViewportSettingsInspector::AddLightingGroup()
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

        auto inspectorWidget = new InspectorPropertyGroupWidget(
            &m_lightingPreset, &m_lightingPreset, m_lightingPreset.TYPEINFO_Uuid(), this, groupWidget, GetGroupSaveStateKey(groupName));

        groupWidget->layout()->addWidget(inspectorWidget);

        AddGroup(groupName, groupDisplayName, groupDescription, groupWidget);
    }

    void EntityPreviewViewportSettingsInspector::CreateLightingPreset()
    {
        const AZStd::string defaultPath = GetUniqueDefaultSaveFilePath(AZ::Render::LightingPreset::Extension);
        const AZStd::string savePath = GetSaveFilePath(defaultPath);
        if (!savePath.empty())
        {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId,
                [&savePath](EntityPreviewViewportSettingsRequestBus::Events* viewportRequests)
                {
                    viewportRequests->SetLightingPreset(AZ::Render::LightingPreset());
                    viewportRequests->SaveLightingPreset(savePath);
                });
        }
    }

    void EntityPreviewViewportSettingsInspector::SelectLightingPreset()
    {
        const int itemSize = aznumeric_cast<int>(GetSettingsValue<AZ::u64>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettingsInspector/AssetSelectionGrid/LightingItemSize", 128));

        AssetSelectionGrid dialog("Lighting Preset Browser", [](const AZ::Data::AssetInfo& assetInfo) {
            return assetInfo.m_assetType == AZ::RPI::AnyAsset::RTTI_Type() &&
                AZ::StringFunc::EndsWith(assetInfo.m_relativePath.c_str(), AZ::Render::LightingPreset::Extension);
        }, QSize(itemSize, itemSize), GetToolMainWindow());

        AZ::Data::AssetId assetId;
        EntityPreviewViewportSettingsRequestBus::EventResult(
            assetId, m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::GetLastLightingPresetAssetId);
        dialog.SelectAsset(assetId);

        connect(&dialog, &AssetSelectionGrid::AssetRejected, this, [this, assetId]() {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadLightingPresetByAssetId, assetId);
        });

        connect(&dialog, &AssetSelectionGrid::AssetSelected, this, [this](const AZ::Data::AssetId& assetId) {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadLightingPresetByAssetId, assetId);
        });

        dialog.setFixedSize(800, 400);
        dialog.show();

        // Removing fixed size to allow drag resizing
        dialog.setMinimumSize(0, 0);
        dialog.setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        dialog.exec();
    }

    void EntityPreviewViewportSettingsInspector::SaveLightingPreset()
    {
        AZStd::string defaultPath;
        EntityPreviewViewportSettingsRequestBus::EventResult(
            defaultPath, m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::GetLastLightingPresetPath);

        if (defaultPath.empty())
        {
            defaultPath = GetUniqueDefaultSaveFilePath(AZ::Render::LightingPreset::Extension);
        }

        const AZStd::string savePath = GetSaveFilePath(defaultPath);
        if (!savePath.empty())
        {
            EntityPreviewViewportSettingsRequestBus::Event(m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SetLightingPreset, m_lightingPreset);
            EntityPreviewViewportSettingsRequestBus::Event(m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SaveLightingPreset, savePath);
        }
    }

    void EntityPreviewViewportSettingsInspector::SaveSettings()
    {
        EntityPreviewViewportSettingsRequestBus::Event(
            m_toolId,
            [this](EntityPreviewViewportSettingsRequestBus::Events* viewportRequests)
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

    void EntityPreviewViewportSettingsInspector::LoadSettings()
    {
        EntityPreviewViewportSettingsRequestBus::Event(
            m_toolId,
            [this](EntityPreviewViewportSettingsRequestBus::Events* viewportRequests)
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

    void EntityPreviewViewportSettingsInspector::Reset()
    {
        LoadSettings();
        InspectorWidget::Reset();
    }

    void EntityPreviewViewportSettingsInspector::OnViewportSettingsChanged()
    {
        LoadSettings();
        RefreshAll();
    }

    void EntityPreviewViewportSettingsInspector::AfterPropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode)
    {
        SaveSettings();
    }

    void EntityPreviewViewportSettingsInspector::SetPropertyEditingComplete([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode)
    {
        SaveSettings();
    }

    AZ::Crc32 EntityPreviewViewportSettingsInspector::GetGroupSaveStateKey(const AZStd::string& groupName) const
    {
        return AZ::Crc32(AZStd::string::format("/O3DE/AtomToolsFramework/EntityPreviewViewportSettingsInspector/PropertyGroup/%s", groupName.c_str()));
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/EntityPreviewViewport/moc_EntityPreviewViewportSettingsInspector.cpp>
