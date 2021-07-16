/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "ConnectionEditDialog.h"

#include <AzQtComponents/Components/Widgets/SpinBox.h>

#include "../connection/connectionManager.h"


#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>

static QVariant dataAtColumn(const QModelIndex& index, int column)
{
    QModelIndex columnIndex = index.sibling(index.row(), column);
    return columnIndex.data(Qt::DisplayRole);
}

void setDataAtColumn(ConnectionManager* connectionManager, const QModelIndex& index, int column, const QVariant& data)
{
    QModelIndex columnIndex = index.sibling(index.row(), column);
    connectionManager->setData(columnIndex, data, Qt::DisplayRole);
}

template <typename WidgetType>
WidgetType* createGridRowWidget(QGridLayout* gridLayout, int gridRow, QDialog* parent, const QString& label)
{
    QLabel* labelWidget = new QLabel(label, parent);
    gridLayout->addWidget(labelWidget, gridRow, 0, Qt::AlignRight);

    WidgetType* widget = new WidgetType(parent);
    gridLayout->addWidget(widget, gridRow, 1, Qt::AlignLeft);

    return widget;
}

ConnectionEditDialog::ConnectionEditDialog(ConnectionManager* connectionManager, const QModelIndex& connectionIndex, QWidget* parent)
    : AzQtComponents::StyledDialog(parent)
    , m_connectionManager(connectionManager)
    , m_index(connectionIndex)
{
    setWindowTitle("Edit Connection");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addSpacing(16);

    QGridLayout* gridLayout = new QGridLayout(this);

    int row = 0;
    m_id = createGridRowWidget<QLineEdit>(gridLayout, row++, this, tr("ID"));
    m_id->setPlaceholderText("Enter a name");
    m_id->setText(dataAtColumn(connectionIndex, ConnectionManager::IdColumn).toString());

    m_ipAddress = createGridRowWidget<QLineEdit>(gridLayout, row++, this, tr("IP Address"));
    m_ipAddress->setPlaceholderText("Enter an IP address");
    m_ipAddress->setText(dataAtColumn(connectionIndex, ConnectionManager::IpColumn).toString());
    
    using namespace AzQtComponents;
    m_port = createGridRowWidget<SpinBox>(gridLayout, row++, this, tr("Port"));
    m_port->setMinimum(0);
    m_port->setMaximum(std::numeric_limits<unsigned short>::max());
    m_port->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_port->setValue(dataAtColumn(connectionIndex, ConnectionManager::PortColumn).toInt());

    layout->addLayout(gridLayout);

    layout->addSpacing(16);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok) | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    adjustSize();
}

void ConnectionEditDialog::accept()
{
    // since this is a modal dialog, and since the user created indices should only 
    // be edited by the user, this should always work
    Q_ASSERT(m_index.isValid());

    if (m_index.isValid())
    {
        setDataAtColumn(m_connectionManager, m_index, ConnectionManager::IdColumn, m_id->text());
        setDataAtColumn(m_connectionManager, m_index, ConnectionManager::IpColumn, m_ipAddress->text());
        setDataAtColumn(m_connectionManager, m_index, ConnectionManager::PortColumn, m_port->value());
    }

    AzQtComponents::StyledDialog::accept();
}


