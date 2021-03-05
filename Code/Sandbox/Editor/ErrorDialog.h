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
#include <QDialog>

#include "Include/EditorCoreAPI.h"
#endif

namespace Ui 
{
    class ErrorLogDialog;
}
class QItemSelection;

namespace SandboxEditor
{
    /// Used to display a collection of error and warning messages.
    /// This is used instead of QMessageBox because the details section of QMessageBox is not
    /// very resizeable, making it hard to show multiple errors at once.
    class EDITOR_CORE_API ErrorDialog
        : public QDialog
    {
        Q_OBJECT
    public:
        enum class MessageType
        {
            Warning,
            Error
        };
        
        //! ErrorDialog constructor.
        //! @param parent the optional QWidget to use as a parent.
        explicit ErrorDialog(QWidget* parent = nullptr);
        ~ErrorDialog();

        //! Adds messages to the dialog, marked with the passed in message type.
        //! @param messageType what message type to use to mark passed in messages.
        //! @param messages the list of messages to add.
        void AddMessages(MessageType messageType, const AZStd::list<QString>& messages);

    protected Q_SLOTS:
        void OnOK();
        void MessageSelectionChanged();

    private:
        /// Tracks which column the message information is used in.
        enum class MessageColumn
        {
            MessageType,
            ShortMessage,
            DetailedMessage,
        };

        //! Converts MessageType to QString.
        //! @param messageType the message type to convert.
        //! @return the QString representation of that message type.
        QString GetMessageTypeString(MessageType messageType) const;

        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        QScopedPointer<Ui::ErrorLogDialog> m_ui; ///< Tracks the Qt UI associated with this class.
        QSet<QString> m_uniqueStrings;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    };
}
