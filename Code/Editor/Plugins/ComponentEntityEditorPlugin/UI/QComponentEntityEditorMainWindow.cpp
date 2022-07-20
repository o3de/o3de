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

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>

#include <QVBoxLayout>

QComponentEntityEditorInspectorWindow::QComponentEntityEditorInspectorWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_propertyEditor(nullptr)
{
    setObjectName("InspectorMainWindow");
    gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);

    Init();
}

QComponentEntityEditorInspectorWindow::~QComponentEntityEditorInspectorWindow()
{
    gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
}

void QComponentEntityEditorInspectorWindow::OnSystemEvent([[maybe_unused]] ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
{
}

void QComponentEntityEditorInspectorWindow::Init()
{
    QVBoxLayout* layout = new QVBoxLayout();

    m_propertyEditor = new AzToolsFramework::EntityPropertyEditor(nullptr);
    layout->addWidget(m_propertyEditor);

    QWidget* window = new QWidget();
    window->setLayout(layout);
    setCentralWidget(window);
}

///////////////////////////////////////////////////////////////////////////////
// End of context menu handling
///////////////////////////////////////////////////////////////////////////////

#include <UI/moc_QComponentEntityEditorMainWindow.cpp>

