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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorDefs.h"

#include "MatEditPreviewDlg.h"

// Qt
#include <QFileDialog>
#include <QMenuBar>
#include <QVBoxLayout>

// Editor
#include "Material/MaterialManager.h"
#include "Material/MaterialPreviewModelView.h"


/////////////////////////////////////////////////////////////////////////////
// CMatEditPreviewDlg dialog


CMatEditPreviewDlg::CMatEditPreviewDlg(QWidget* parent)
    : QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setWindowTitle(tr("Material Preview"));

    /* create sub controls */
    m_previewCtrl.reset(new MaterialPreviewModelView(this));
    m_menubar.reset(new QMenuBar);

    /* configure layout */
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_menubar.data());
    layout->addWidget(m_previewCtrl.data());
    layout->setStretchFactor(m_previewCtrl.data(), 1);
    setLayout(layout);

    GetIEditor()->GetMaterialManager()->AddListener(this);

    SetupMenuBar();

    OnPreviewPlane();
    m_previewCtrl->SetMaterial(GetIEditor()->GetMaterialManager()->GetCurrentMaterial() ? GetIEditor()->GetMaterialManager()->GetCurrentMaterial()->GetMatInfo() : nullptr);
    m_previewCtrl->Update();
}

CMatEditPreviewDlg::~CMatEditPreviewDlg()
{
    GetIEditor()->GetMaterialManager()->RemoveListener(this);
}

QSize CMatEditPreviewDlg::sizeHint() const
{
    return QSize(450, 400);
}

void CMatEditPreviewDlg::showEvent(QShowEvent* e)
{
    QDialog::showEvent(e);
    resize(sizeHint()); // Because WindowDecorationWrapper resizes it to the minimum for some reason.
}

//////////////////////////////////////////////////////////////////////////
void CMatEditPreviewDlg::SetupMenuBar()
{
    QMenu* menu;
    QAction* action;

    menu = m_menubar->addMenu(tr("Preview"));
    action = menu->addAction(tr("&Plane"));
    connect(action, &QAction::triggered, this, &CMatEditPreviewDlg::OnPreviewPlane);
    action = menu->addAction(tr("&Sphere"));
    connect(action, &QAction::triggered, this, &CMatEditPreviewDlg::OnPreviewSphere);
    action = menu->addAction(tr("&Box"));
    connect(action, &QAction::triggered, this, &CMatEditPreviewDlg::OnPreviewBox);
    action = menu->addAction(tr("&Teapot"));
    connect(action, &QAction::triggered, this, &CMatEditPreviewDlg::OnPreviewTeapot);
    action = menu->addAction(tr("&Custom"));
    connect(action, &QAction::triggered, this, &CMatEditPreviewDlg::OnPreviewCustom);
}

//////////////////////////////////////////////////////////////////////////
void CMatEditPreviewDlg::OnPreviewSphere()
{
    m_previewCtrl->LoadModelFile("Editor/Objects/MtlSphere.cgf");
}

//////////////////////////////////////////////////////////////////////////
void CMatEditPreviewDlg::OnPreviewBox()
{
    m_previewCtrl->LoadModelFile("Editor/Objects/MtlBox.cgf");
}

//////////////////////////////////////////////////////////////////////////
void CMatEditPreviewDlg::OnPreviewTeapot()
{
    m_previewCtrl->LoadModelFile("Editor/Objects/MtlTeapot.cgf");
}

//////////////////////////////////////////////////////////////////////////
void CMatEditPreviewDlg::OnPreviewPlane()
{
    m_previewCtrl->LoadModelFile("Editor/Objects/MtlPlane.cgf");
}

//////////////////////////////////////////////////////////////////////////
void CMatEditPreviewDlg::OnPreviewCustom()
{
    const QString fullFileName =
        QFileDialog::getOpenFileName(this, tr("Custom Model"), QString(), tr("Objects (*.cgf);;All files (*.*)"));
    if (!fullFileName.isNull())
    {
        m_previewCtrl->LoadModelFile(fullFileName);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CMatEditPreviewDlg message handlers

void CMatEditPreviewDlg::OnDataBaseItemEvent([[maybe_unused]] IDataBaseItem* pItem, EDataBaseItemEvent event)
{
    switch (event)
    {
    case EDB_ITEM_EVENT_SELECTED:
    case EDB_ITEM_EVENT_ADD:
    case EDB_ITEM_EVENT_CHANGED:
        m_previewCtrl->SetMaterial(GetIEditor()->GetMaterialManager()->GetCurrentMaterial() ? GetIEditor()->GetMaterialManager()->GetCurrentMaterial()->GetMatInfo() : nullptr);
        break;
    case EDB_ITEM_EVENT_DELETE:
        m_previewCtrl->SetMaterial(nullptr);
        break;
    }
}

#include <moc_MatEditPreviewDlg.cpp>
