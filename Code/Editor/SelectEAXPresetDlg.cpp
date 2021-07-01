/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
