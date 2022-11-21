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
#include <QtConcurrent/QtConcurrent>

namespace AtomToolsFramework
{
    EntityPreviewViewportSettingsInspector::EntityPreviewViewportSettingsInspector(const AZ::Crc32& toolId, QWidget* parent)
        : InspectorWidget(parent)
        , m_toolId(toolId)
    {
        setObjectName("EntityPreviewViewportSettingsInspector");
        SetGroupSettingsPrefix("/O3DE/AtomToolsFramework/EntityPreviewViewportSettingsInspector");
        Populate();

        // Pre create the model preset dialog so that it is not repopulated every time it's opened.
        const int modelPresetDialogItemSize = aznumeric_cast<int>(GetSettingsValue<AZ::u64>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettingsInspector/AssetSelectionGrid/ModelItemSize", 128));

        m_modelPresetDialog.reset(new AssetSelectionGrid("Model Preset Browser", [](const AZStd::string& path) {
            return path.ends_with(AZ::Render::ModelPreset::Extension);
        }, QSize(modelPresetDialogItemSize, modelPresetDialogItemSize), GetToolMainWindow()));

        // Pre create the lighting preset dialog so that it is not repopulated every time it's opened.
        const int lightingPresetDialogItemSize = aznumeric_cast<int>(GetSettingsValue<AZ::u64>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettingsInspector/AssetSelectionGrid/LightingItemSize", 128));

        m_lightingPresetDialog.reset(new AssetSelectionGrid("Lighting Preset Browser", [](const AZStd::string& path) {
            return path.ends_with(AZ::Render::LightingPreset::Extension);
        }, QSize(lightingPresetDialogItemSize, lightingPresetDialogItemSize), GetToolMainWindow()));

        // Add the last known paths for lighting and model presets to the browsers so they are not empty while the rest of the data is
        // being processed in the background.
        EntityPreviewViewportSettingsRequestBus::Event(
            m_toolId,
            [this](EntityPreviewViewportSettingsRequests* viewportRequests)
            {
                m_lightingPresetDialog->AddPath(viewportRequests->GetLastLightingPresetPath());
                m_modelPresetDialog->AddPath(viewportRequests->GetLastModelPresetPath());
            });

        // Using a future watcher to monitor a background process that enumerates all of the lighting and model presets in the project.
        // After the background process is complete, the watcher will receive the finished signal and add all of the enumerated files to
        // the browsers.
        connect(&m_watcher, &QFutureWatcher<AZStd::vector<AZStd::string>>::finished, this, [this](){
            m_lightingPresetDialog->Clear();
            m_modelPresetDialog->Clear();
            for (const auto& path : m_watcher.result())
            {
                m_lightingPresetDialog->AddPath(path);
                m_modelPresetDialog->AddPath(path);
            }

            connect(m_modelPresetDialog.get(), &AssetSelectionGrid::PathRejected, this, [this]() {
                EntityPreviewViewportSettingsRequestBus::Event(
                    m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadModelPreset, m_modelPresetPath);
            });

            connect(m_modelPresetDialog.get(), &AssetSelectionGrid::PathSelected, this, [this](const AZStd::string& path) {
                EntityPreviewViewportSettingsRequestBus::Event(
                    m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadModelPreset, path);
            });

            connect(m_lightingPresetDialog.get(), &AssetSelectionGrid::PathRejected, this, [this]() {
                EntityPreviewViewportSettingsRequestBus::Event(
                    m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadLightingPreset, m_lightingPresetPath);
            });

            connect(m_lightingPresetDialog.get(), &AssetSelectionGrid::PathSelected, this, [this](const AZStd::string& path) {
                EntityPreviewViewportSettingsRequestBus::Event(
                    m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadLightingPreset, path);
            });

            OnViewportSettingsChanged();
        });

        m_watcher.setFuture(QtConcurrent::run([]() {
            return GetPathsInSourceFoldersMatchingFilter([](const AZStd::string& path) {
                return path.ends_with(AZ::Render::LightingPreset::Extension) || path.ends_with(AZ::Render::ModelPreset::Extension);
            });
        }));

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
        const AZStd::string savePath =
            GetSaveFilePathFromDialog({}, { { "Model Preset", AZ::Render::ModelPreset::Extension } }, "Model Preset");
        if (!savePath.empty())
        {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SetModelPreset, AZ::Render::ModelPreset());
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SaveModelPreset, savePath);
        }
    }

    void EntityPreviewViewportSettingsInspector::SelectModelPreset()
    {
        EntityPreviewViewportSettingsRequestBus::EventResult(
            m_modelPresetPath, m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::GetLastModelPresetPath);

        m_modelPresetDialog->SelectPath(m_modelPresetPath);
        m_modelPresetDialog->setFixedSize(800, 400);
        m_modelPresetDialog->show();

        // Removing fixed size to allow drag resizing
        m_modelPresetDialog->setMinimumSize(0, 0);
        m_modelPresetDialog->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        m_modelPresetDialog->exec();
    }

    void EntityPreviewViewportSettingsInspector::SaveModelPreset()
    {
        AZStd::string defaultPath;
        EntityPreviewViewportSettingsRequestBus::EventResult(
            defaultPath, m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::GetLastModelPresetPath);

        const AZStd::string savePath =
            GetSaveFilePathFromDialog(defaultPath, { { "Model Preset", AZ::Render::ModelPreset::Extension } }, "Model Preset");
        if (!savePath.empty())
        {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SetModelPreset, m_modelPreset);
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SaveModelPreset, savePath);
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
        const AZStd::string savePath =
            GetSaveFilePathFromDialog({}, { { "Lighting Preset", AZ::Render::LightingPreset::Extension } }, "Lighting Preset");
        if (!savePath.empty())
        {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SetLightingPreset, AZ::Render::LightingPreset());
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SaveLightingPreset, savePath);
        }
    }

    void EntityPreviewViewportSettingsInspector::SelectLightingPreset()
    {
        EntityPreviewViewportSettingsRequestBus::EventResult(
            m_lightingPresetPath, m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::GetLastLightingPresetPath);

        m_lightingPresetDialog->SelectPath(m_lightingPresetPath);
        m_lightingPresetDialog->setFixedSize(800, 400);
        m_lightingPresetDialog->show();

        // Removing fixed size to allow drag resizing
        m_lightingPresetDialog->setMinimumSize(0, 0);
        m_lightingPresetDialog->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        m_lightingPresetDialog->exec();
    }

    void EntityPreviewViewportSettingsInspector::SaveLightingPreset()
    {
        AZStd::string defaultPath;
        EntityPreviewViewportSettingsRequestBus::EventResult(
            defaultPath, m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::GetLastLightingPresetPath);

        const AZStd::string savePath =
            GetSaveFilePathFromDialog(defaultPath, { { "Lighting Preset", AZ::Render::LightingPreset::Extension } }, "Lighting Preset");
        if (!savePath.empty())
        {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SetLightingPreset, m_lightingPreset);
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SaveLightingPreset, savePath);
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

    void EntityPreviewViewportSettingsInspector::OnModelPresetAdded(const AZStd::string& path)
    {
        m_modelPresetDialog->AddPath(path);
    }

    void EntityPreviewViewportSettingsInspector::OnLightingPresetAdded(const AZStd::string& path)
    {
        m_lightingPresetDialog->AddPath(path);
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
