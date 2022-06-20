/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "LayoutConfigDialog.h"

// AzQtComponents
#include <AzQtComponents/Components/StyleManager.h>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_LayoutConfigDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

class LayoutConfigModel
    : public QAbstractListModel
{
public:
    LayoutConfigModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

private:
    static const int kNumLayouts = 9;
};

LayoutConfigModel::LayoutConfigModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int LayoutConfigModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : kNumLayouts;
}

int LayoutConfigModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 1;
}

QVariant LayoutConfigModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.column() > 0 || index.row() >= kNumLayouts)
    {
        return {};
    }

    switch (role)
    {
    case Qt::SizeHintRole:
        return QSize(38, 38);

    case Qt::DecorationRole:
        return QPixmap(QStringLiteral(":/layouts/layouts-%1.svg").arg(index.row()));
    }

    return {};
}

// CLayoutConfigDialog dialog

CLayoutConfigDialog::CLayoutConfigDialog(QWidget* pParent /*=nullptr*/)
    : QDialog(pParent)
    , m_model(new LayoutConfigModel(this))
    , ui(new Ui::CLayoutConfigDialog)
{
    m_layout = ET_Layout1;

    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(size());

    ui->m_layouts->setModel(m_model);

    AzQtComponents::StyleManager::setStyleSheet(ui->m_layouts, "style:LayoutConfigDialog.qss");

    connect(ui->m_buttonBox, &QDialogButtonBox::accepted, this, &CLayoutConfigDialog::OnOK);
    connect(ui->m_buttonBox, &QDialogButtonBox::rejected, this, &CLayoutConfigDialog::reject);
}

CLayoutConfigDialog::~CLayoutConfigDialog()
{
}

void CLayoutConfigDialog::SetLayout(EViewLayout layout)
{
    m_layout = layout;
    ui->m_layouts->setCurrentIndex(m_model->index(static_cast<int>(layout), 0));
}

// CLayoutConfigDialog message handlers

//////////////////////////////////////////////////////////////////////////
void CLayoutConfigDialog::OnOK()
{
    auto index = ui->m_layouts->currentIndex();
    auto oldLayout = m_layout;

    if (index.isValid())
    {
        m_layout = static_cast<EViewLayout>(index.row());
    }

    // If the layout has not been changed, the calling code
    // is not supposed to do anything. So let's simply reject in that case.
    done(m_layout == oldLayout ? QDialog::Rejected : QDialog::Accepted);
}

#include <moc_LayoutConfigDialog.cpp>
