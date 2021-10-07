/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "ResizeResolutionDialog.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_ResizeResolutionDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

class ResizeResolutionModel
    : public QAbstractListModel
{
public:
    ResizeResolutionModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    int SizeRow(uint32 dwSize) const;

private:
    static const int kNumSizes = 6;
};

ResizeResolutionModel::ResizeResolutionModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int ResizeResolutionModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : kNumSizes;
}

int ResizeResolutionModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 1;
}

QVariant ResizeResolutionModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.column() > 0 || index.row() >= kNumSizes)
    {
        return {};
    }

    const int size = 64 * (1 << index.row());

    switch (role)
    {
    case Qt::DisplayRole:
        return QStringLiteral("%1x%2").arg(size).arg(size);

    case Qt::UserRole:
        return size;
    }

    return {};
}

int ResizeResolutionModel::SizeRow(uint32 dwSize) const
{
    // not a power of 2?
    if (dwSize & (dwSize - 1))
    {
        return 0;
    }

    int row = 0;

    for (auto i = dwSize / 64; i > 1; i >>= 1)
    {
        ++row;
    }

    return row;
}

/////////////////////////////////////////////////////////////////////////////
// CResizeResolutionDialog dialog


CResizeResolutionDialog::CResizeResolutionDialog(QWidget* pParent /*=nullptr*/)
    : QDialog(pParent)
    , m_model(new ResizeResolutionModel(this))
    , ui(new Ui::CResizeResolutionDialog)
{
    ui->setupUi(this);

    ui->m_resolution->setModel(m_model);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

CResizeResolutionDialog::~CResizeResolutionDialog()
{
}

/////////////////////////////////////////////////////////////////////////////
void CResizeResolutionDialog::SetSize(uint32 dwSize)
{
    ui->m_resolution->setCurrentIndex(m_model->SizeRow(dwSize));
}

/////////////////////////////////////////////////////////////////////////////
uint32 CResizeResolutionDialog::GetSize()
{
    return ui->m_resolution->itemData(ui->m_resolution->currentIndex()).toInt();
}
