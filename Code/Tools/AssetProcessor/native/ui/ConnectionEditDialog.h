/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <AzQtComponents/Components/StyledDialog.h>
#endif

class ConnectionManager;
class QLineEdit;

namespace AzQtComponents
{
    class SpinBox;
}

class ConnectionEditDialog : public AzQtComponents::StyledDialog
{
    Q_OBJECT // AUTOMOC

public:
    ConnectionEditDialog(ConnectionManager* connectionManager, const QModelIndex& connectionIndex, QWidget* parent = nullptr);

    void accept() override;

private:

    ConnectionManager* m_connectionManager;
    QPersistentModelIndex m_index;

    QLineEdit* m_id;
    QLineEdit* m_ipAddress;
    AzQtComponents::SpinBox* m_port;
};
