/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UI/QComponentEntityEditorMainWindow.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>

#include <QCloseEvent>
#include <QVBoxLayout>

QComponentEntityEditorInspectorWindow::QComponentEntityEditorInspectorWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_propertyEditor(nullptr)
    , m_assetBrowserInspector(nullptr)
    , m_inspectorWidgetStack(nullptr)
{
    setObjectName("InspectorMainWindow");
    gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);
    AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
    AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Handler::BusConnect();

    Init();
}

QComponentEntityEditorInspectorWindow::~QComponentEntityEditorInspectorWindow()
{
    AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Handler::BusDisconnect();
    AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect();
    gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
}

void QComponentEntityEditorInspectorWindow::closeEvent(QCloseEvent* ev)
{
    if (m_assetBrowserInspector && m_assetBrowserInspector->HasUnsavedChanges())
    {
        ev->ignore();
        return;
    }
    ev->accept();
}

void QComponentEntityEditorInspectorWindow::OnSystemEvent([[maybe_unused]] ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
{
}

void QComponentEntityEditorInspectorWindow::AfterEntitySelectionChanged(
    [[maybe_unused]] const AzToolsFramework::EntityIdList& newlySelectedEntities,
    [[maybe_unused]] const AzToolsFramework::EntityIdList& newlyDeselectedEntities)
{
    if (m_inspectorWidgetStack->currentWidget() != m_propertyEditor && !m_propertyEditor->IsLockedToSpecificEntities())
    {
        m_inspectorWidgetStack->setCurrentIndex(m_inspectorWidgetStack->indexOf(m_propertyEditor));
    }
}

void QComponentEntityEditorInspectorWindow::PreviewAsset([[maybe_unused]] const AzToolsFramework::AssetBrowser::AssetBrowserEntry* selectedEntry)
{
    if (m_inspectorWidgetStack->currentWidget() != m_assetBrowserInspector && !m_propertyEditor->IsLockedToSpecificEntities())
    {
        m_inspectorWidgetStack->setCurrentIndex(m_inspectorWidgetStack->indexOf(m_assetBrowserInspector));
    }
}

void QComponentEntityEditorInspectorWindow::Init()
{
    QVBoxLayout* layout = new QVBoxLayout();

    m_inspectorWidgetStack = new QStackedWidget();
    m_inspectorWidgetStack->setContentsMargins(0, 0, 0, 0);
    m_propertyEditor = new AzToolsFramework::EntityPropertyEditor(this);
    m_assetBrowserInspector = new AzToolsFramework::AssetBrowser::AssetBrowserEntityInspectorWidget(this);
    m_inspectorWidgetStack->addWidget(m_propertyEditor);
    m_inspectorWidgetStack->addWidget(m_assetBrowserInspector);
    layout->addWidget(m_inspectorWidgetStack);

    QWidget* window = new QWidget();
    window->setLayout(layout);
    setCentralWidget(window);
}

///////////////////////////////////////////////////////////////////////////////
// End of context menu handling
///////////////////////////////////////////////////////////////////////////////

#include <UI/moc_QComponentEntityEditorMainWindow.cpp>

