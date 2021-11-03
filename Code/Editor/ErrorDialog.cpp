/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "ErrorDialog.h"


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_ErrorDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

namespace SandboxEditor
{
    ErrorDialog::ErrorDialog(QWidget* parent)
        : QDialog(parent)
        , m_ui(new Ui::ErrorLogDialog())
    {
        m_ui->setupUi(this);
        connect(m_ui->okButton, &QPushButton::clicked, this, &ErrorDialog::OnOK);
        connect(
            m_ui->messages, 
            &QTreeWidget::itemSelectionChanged,
            this, 
            &ErrorDialog::MessageSelectionChanged);
    }

    ErrorDialog::~ErrorDialog()
    {
    }

    void ErrorDialog::OnOK()
    {
        close();
    }

    void ErrorDialog::AddMessages(MessageType messageType, const AZStd::list<QString>& messages)
    {
        AZ_Assert(m_ui != nullptr, "ErrorDialog's AddMessages cannot be used without a valid UI.");
        for (const QString& message : messages)
        {
            // filter out duplicate messages
            if (m_uniqueStrings.contains(message))
            {
                continue;
            }
            m_uniqueStrings.insert(message);

            // Attempt to split the message up by newline, so the list of errors can be kept shorter.
            // This returns a QStringList with one element, the original string, if the split fails.
            QStringList messageSplitPerLine = message.split('\n');

            QStringList messageList;

            // MessageColumn::MessageType
            messageList << GetMessageTypeString(messageType);
            // MessageColumn::ShortMessage
            messageList << messageSplitPerLine[0];
            // MessageColumn::DetailedMessage
            messageList << message;

            // Add the message to the tree widget at the root.
            m_ui->messages->insertTopLevelItem(0, new QTreeWidgetItem(messageList));
        }
    }

    void ErrorDialog::MessageSelectionChanged()
    {
        AZ_Assert(m_ui != nullptr, "ErrorDialog's MessageSelectionChanged cannot be used without a valid UI.");
        AZ_Assert(m_ui->messages != nullptr, "ErrorDialog's MessageSelectionChanged cannot be used without a valid messages QTreeWidget.");
        AZ_Assert(m_ui->details != nullptr, "ErrorDialog's MessageSelectionChanged cannot be used without a valid details QLabel.");
        QList<QTreeWidgetItem*> selectedItem = m_ui->messages->selectedItems();
        
        QTreeWidgetItem* firstSelected = !selectedItem.isEmpty() ? selectedItem.first() : nullptr;
        if (firstSelected)
        {
            m_ui->details->setText(firstSelected->text((int)MessageColumn::DetailedMessage));
        }
    }

    QString ErrorDialog::GetMessageTypeString(MessageType messageType) const
    {
        switch (messageType)
        {
        case ErrorDialog::MessageType::Warning:
            return QObject::tr("Warning");
        case MessageType::Error:
            return QObject::tr("Error");
        default:
            {
                AZ_Warning(
                    "ErrorDialog", 
                    false,
                    "No message type string is available for message type %u",
                    messageType);

                return QObject::tr("Unknown");
            }
        }
    }
}



#include <moc_ErrorDialog.cpp>
