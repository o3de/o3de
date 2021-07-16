/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TrackViewFBXImportPreviewDialog.h"

// Qt
#include <QAbstractListModel>


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_TrackViewFBXImportPreviewDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING


class FBXImportModel
    : public QAbstractListModel
{
public:
    FBXImportModel(CTrackViewFBXImportPreviewDialog::TItemsMap& map, QObject* parent = nullptr)
        : QAbstractListModel(parent)
        , m_map(map)
    {
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : m_map.size();
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() >= m_map.size())
        {
            return QVariant();
        }
        auto it = m_map.begin() + index.row();
        switch (role)
        {
        case Qt::DisplayRole:
            return it->name;
        case Qt::CheckStateRole:
            return it->checked ? Qt::Checked : Qt::Unchecked;
        default:
            return QVariant();
        }
    }

    bool setData(const QModelIndex& index, const QVariant& data, int role = Qt::EditRole) override
    {
        if (!index.isValid() || index.row() >= m_map.size())
        {
            return false;
        }
        auto it = m_map.begin() + index.row();
        switch (role)
        {
        case Qt::CheckStateRole:
            it->checked = (data.toInt() == Qt::Checked);
            emit dataChanged(index, index, QVector<int>(1, Qt::CheckStateRole));
            return true;
        default:
            return false;
        }
    }

    void setAllItemsChecked(bool checked)
    {
        if (m_map.empty())
        {
            return;
        }
        for (auto it : m_map)
        {
            it.checked = checked;
        }
        emit dataChanged(index(0, 0), index(m_map.size() - 1, 0), QVector<int>(1, Qt::CheckStateRole));
    }

private:
    CTrackViewFBXImportPreviewDialog::TItemsMap& m_map;
};

//////////////////////////////////////////////////////////////////////////
CTrackViewFBXImportPreviewDialog::CTrackViewFBXImportPreviewDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::TrackViewFBXImportPreviewDialog)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(size());
    connect(m_ui->buttonSelectAll, &QPushButton::clicked, this, &CTrackViewFBXImportPreviewDialog::OnBnSelectAllClicked);
    connect(m_ui->buttonUnselectAll, &QPushButton::clicked, this, &CTrackViewFBXImportPreviewDialog::OnBnUnselectAllClicked);
}

CTrackViewFBXImportPreviewDialog::~CTrackViewFBXImportPreviewDialog()
{
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewFBXImportPreviewDialog::exec()
{
    m_ui->m_tree->setModel(new FBXImportModel(m_fBXItemNames, this));
    return QDialog::exec();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewFBXImportPreviewDialog::AddTreeItem(const QString& objectName)
{
    m_fBXItemNames.push_back(Item { objectName, true });
}

bool CTrackViewFBXImportPreviewDialog::IsObjectSelected(const QString& objectName) const
{
    for (auto it : m_fBXItemNames)
    {
        if (it.name == objectName)
        {
            return it.checked;
        }
    }
    return false;
};

void CTrackViewFBXImportPreviewDialog::OnBnSelectAllClicked()
{
    static_cast<FBXImportModel*>(m_ui->m_tree->model())->setAllItemsChecked(true);
}

void CTrackViewFBXImportPreviewDialog::OnBnUnselectAllClicked()
{
    static_cast<FBXImportModel*>(m_ui->m_tree->model())->setAllItemsChecked(false);
}

#include <moc_TrackViewFBXImportPreviewDialog.cpp>
