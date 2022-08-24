/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <QTextEdit>
#endif // !defined(Q_MOC_RUN)

namespace AzToolsFramework
{
    class DPEDebugTextView : public QTextEdit
    {
        Q_OBJECT

    public:
        DPEDebugTextView(QWidget* parent = nullptr);

        void SetAdapter(AZ::DocumentPropertyEditor::DocumentAdapterPtr adapter);

    private:
        void UpdateContents();

        AZ::DocumentPropertyEditor::DocumentAdapterPtr m_adapter;
        AZ::DocumentPropertyEditor::DocumentAdapter::ResetEvent::Handler m_resetHandler;
        AZ::DocumentPropertyEditor::DocumentAdapter::ChangedEvent::Handler m_changeHandler;
    };
} // namespace AzToolsFramework
