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