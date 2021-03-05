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

#include "SelectEAXPresetDlg.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include "ui_SelectEAXPresetDlg.h"
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING


CSelectEAXPresetDlg::CSelectEAXPresetDlg(QWidget* pParent)
    : QDialog(pParent)
    , m_ui(new Ui_CSelectEAXPresetDlg)
{
    m_ui->setupUi(this);
}

CSelectEAXPresetDlg::~CSelectEAXPresetDlg()
{
}

void CSelectEAXPresetDlg::SetCurrPreset(const QString& sPreset)
{
    QAbstractListModel* model = Model();
    if (!model)
    {
        return;
    }

    QModelIndexList indexes = model->match(QModelIndex(), Qt::DisplayRole, sPreset, 1, Qt::MatchExactly);

    if (!indexes.isEmpty())
    {
        m_ui->listView->setCurrentIndex(indexes.at(0));
    }
}

QString CSelectEAXPresetDlg::GetCurrPreset() const
{
    if (m_ui->listView->currentIndex().isValid())
    {
        return m_ui->listView->currentIndex().data().toString();
    }
    // EXCEPTION: OCX Property Pages should return FALSE
    return QString();
}


void CSelectEAXPresetDlg::SetModel(QAbstractListModel* model)
{
    m_ui->listView->setModel(model);
}

QAbstractListModel* CSelectEAXPresetDlg::Model() const
{
    return static_cast<QAbstractListModel*>(m_ui->listView->model());
}

#include "moc_SelectEAXPresetDlg.cpp"
