/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UI/QComponentLevelEntityEditorMainWindow.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <QVBoxLayout>

QComponentLevelEntityEditorInspectorWindow::QComponentLevelEntityEditorInspectorWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_propertyEditor(nullptr)
{
    GetIEditor()->RegisterNotifyListener(this);
    AzToolsFramework::SliceMetadataEntityContextNotificationBus::Handler::BusConnect();

    Init();
}

QComponentLevelEntityEditorInspectorWindow::~QComponentLevelEntityEditorInspectorWindow()
{
    AzToolsFramework::SliceMetadataEntityContextNotificationBus::Handler::BusDisconnect();
    GetIEditor()->UnregisterNotifyListener(this);
}

void QComponentLevelEntityEditorInspectorWindow::Init()
{
    QVBoxLayout* layout = new QVBoxLayout();

    m_propertyEditor = new AzToolsFramework::EntityPropertyEditor(nullptr, Qt::WindowFlags(), true);
    layout->addWidget(m_propertyEditor);

    // On initialization, notify our property editor about the root metadata entity if it exists
    RefreshPropertyEditor();

    QWidget* window = new QWidget();
    window->setLayout(layout);
    setCentralWidget(window);
}

void QComponentLevelEntityEditorInspectorWindow::OnMetadataEntityAdded(AZ::EntityId entityId)
{
    AZ::EntityId rootSliceMetaDataEntity = GetRootMetaDataEntityId();
    if (rootSliceMetaDataEntity == entityId)
    {
        AzToolsFramework::EntityIdSet entities;
        entities.insert(rootSliceMetaDataEntity);
        m_propertyEditor->SetOverrideEntityIds(entities);
    }
}

void QComponentLevelEntityEditorInspectorWindow::RefreshPropertyEditor()
{
    AZ::EntityId rootSliceMetaDataEntity = GetRootMetaDataEntityId();
    OnMetadataEntityAdded(rootSliceMetaDataEntity);
}

AZ::EntityId QComponentLevelEntityEditorInspectorWindow::GetRootMetaDataEntityId() const
{
    AZ::EntityId levelEntityId;
    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(levelEntityId, &AzToolsFramework::ToolsApplicationRequests::GetCurrentLevelEntityId);
    return levelEntityId;
}

void QComponentLevelEntityEditorInspectorWindow::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
            // Refresh the Level Component Property Editor any time we start or end
            // a level creation or load.
            case eNotify_OnBeginLoad:
            case eNotify_OnEndLoad:
            case eNotify_OnBeginCreate:
            case eNotify_OnEndCreate:
                RefreshPropertyEditor();
                break;
            default:
                break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// End of context menu handling
///////////////////////////////////////////////////////////////////////////////

#include <UI/moc_QComponentLevelEntityEditorMainWindow.cpp>

