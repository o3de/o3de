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

// Description : CViewportTitleDlg implementation file


#include "ViewportTitleDlg.h"

#include <AzQtComponents/StyleGallery/ui_ViewportTitleDlg.h>
#include <QAction>

ViewportTitleDlg::ViewportTitleDlg(QWidget* pParent)
    : QWidget(pParent),
    m_ui(new Ui::ViewportTitleDlg)
{
    m_ui->setupUi(this);

    OnInitDialog();
    m_ui->m_viewportSearch->setIcon(QIcon(":/stylesheet/img/16x16/Search.png"));

    auto clearAction = new QAction(this);
    clearAction->setIcon(QIcon(":/stylesheet/img/16x16/lineedit-clear.png"));

    m_ui->m_viewportSearch->setFixedWidth(190);
}

ViewportTitleDlg::~ViewportTitleDlg()
{
}

void ViewportTitleDlg::OnInitDialog()
{
    m_ui->m_titleBtn->setText(m_title);
    m_ui->m_sizeStaticCtrl->setText(QString());

    connect(m_ui->m_maxBtn, &QToolButton::clicked, this, &ViewportTitleDlg::OnMaximize);
    connect(m_ui->m_toggleHelpersBtn, &QToolButton::clicked, this, &ViewportTitleDlg::OnToggleHelpers);
    connect(m_ui->m_toggleDisplayInfoBtn, &QToolButton::clicked, this, &ViewportTitleDlg::OnToggleDisplayInfo);

    m_ui->m_maxBtn->setProperty("class", "big");
    m_ui->m_toggleHelpersBtn->setProperty("class", "big");
    m_ui->m_toggleDisplayInfoBtn->setProperty("class", "big");
}

void ViewportTitleDlg::SetTitle( const QString &title )
{
    m_title = title;
    m_ui->m_titleBtn->setText(m_title);
    m_ui->m_viewportSearch->setVisible(title == QLatin1String("Perspective"));
}

void ViewportTitleDlg::OnMaximize()
{
}

void ViewportTitleDlg::OnToggleHelpers()
{
}

void ViewportTitleDlg::OnToggleDisplayInfo()
{
}

#include "StyleGallery/moc_ViewportTitleDlg.cpp"
