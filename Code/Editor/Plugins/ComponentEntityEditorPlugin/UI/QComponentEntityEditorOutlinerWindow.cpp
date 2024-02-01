/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorDefs.h>
#include "CryEdit.h"
#include "UI/QComponentEntityEditorOutlinerWindow.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzToolsFramework/UI/Outliner/EntityOutlinerWidget.hxx>

#include <QVBoxLayout>

QEntityOutlinerWindow::QEntityOutlinerWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_outlinerWidget(nullptr)
{
    gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);

    Init();
}

QEntityOutlinerWindow::~QEntityOutlinerWindow()
{
    gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
}

void QEntityOutlinerWindow::OnSystemEvent([[maybe_unused]] ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
{
}

void QEntityOutlinerWindow::Init()
{
    QVBoxLayout* layout = new QVBoxLayout();

    m_outlinerWidget = new AzToolsFramework::EntityOutlinerWidget(nullptr);
    layout->addWidget(m_outlinerWidget);

    QWidget* window = new QWidget();
    window->setLayout(layout);
    setCentralWidget(window);
}

