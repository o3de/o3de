/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzToolsFramework/UI/DPEDebugViewer/DPEDebugModel.h>
#include <AzToolsFramework/UI/DPEDebugViewer/DPEDebugWindow.h>
#include <AzToolsFramework/UI/DPEDebugViewer/ui_DPEDebugWindow.h>

namespace AzToolsFramework
{
    DPEDebugWindow::DPEDebugWindow(QWidget* parentWidget)
        : QMainWindow(parentWidget)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        m_ui = new Ui::DPEDebugWindow;
        m_ui->setupUi(this);

        m_model = new AzToolsFramework::DPEDebugModel(this);
        m_ui->m_treeView->setModel(m_model);

        QObject::connect(m_ui->adapterSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [&]()
            {
                SetAdapter(m_adapters[m_ui->adapterSelector->currentIndex()]);
            });
    }

    DPEDebugWindow::~DPEDebugWindow() = default;

    void DPEDebugWindow::SetAdapter(AZ::DocumentPropertyEditor::DocumentAdapterPtr adapter)
    {
        if (m_model->GetAdapter() != adapter)
        {
            m_model->SetAdapter(adapter);
            m_ui->m_textView->SetAdapter(adapter);

            for (int columnIndex = 0, maxColumns = m_model->GetMaxColumns(); columnIndex < maxColumns; ++columnIndex)
            {
                // resize the columns to accommodate the displayed data
                m_ui->m_treeView->resizeColumnToContents(columnIndex);
            }
            emit AdapterChanged(adapter);
        }
    }

    AZ::DocumentPropertyEditor::DocumentAdapterPtr DPEDebugWindow::GetAdapter()
    {
        return m_model->GetAdapter();
    }

    void DPEDebugWindow::AddAdapterToList(
        const QString& adapterName, AZStd::shared_ptr<AZ::DocumentPropertyEditor::DocumentAdapter> adapter)
    {
        m_adapters.push_back(adapter);
        m_ui->adapterSelector->addItem(adapterName);
    }

} // namespace AzToolsFramework
