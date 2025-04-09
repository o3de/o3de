/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QMainWindow>
#endif // Q_MOC_RUN

namespace Ui
{
    class DPEDebugWindow;
}

namespace AZ
{
    namespace DocumentPropertyEditor
    {
        class DocumentAdapter;
    }
}

namespace AzToolsFramework
{
    class DPEDebugModel;

    class DPEDebugWindow
        : public QMainWindow
    {
        Q_OBJECT
    public:
        explicit DPEDebugWindow(QWidget* parentWidget);
        ~DPEDebugWindow() override;
        void SetAdapter(AZ::DocumentPropertyEditor::DocumentAdapterPtr adapter);
        AZ::DocumentPropertyEditor::DocumentAdapterPtr GetAdapter();
        void AddAdapterToList(const QString& adapterName, AZ::DocumentPropertyEditor::DocumentAdapterPtr adapter);

    signals:
        void AdapterChanged(AZ::DocumentPropertyEditor::DocumentAdapterPtr newAdapter);

    protected:
        Ui::DPEDebugWindow* m_ui = nullptr;
        DPEDebugModel* m_model = nullptr;
        AZStd::vector<AZ::DocumentPropertyEditor::DocumentAdapterPtr> m_adapters;
    };
}
