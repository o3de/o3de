/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FilteredDPE.h"
#include "AzToolsFramework/UI/DocumentPropertyEditor/ui_FilteredDPE.h"

namespace AzToolsFramework
{
    FilteredDPE::FilteredDPE(QWidget* parentWidget)
        : QWidget(parentWidget)
        , m_filterAdapter(AZStd::make_shared<AZ::DocumentPropertyEditor::ValueStringFilter>())
        , m_ui(new Ui::FilteredDPE())
    {
        m_ui->setupUi(this);
        setAttribute(Qt::WA_DeleteOnClose);
        
        QObject::connect(
            m_ui->m_searchBox,
            &QLineEdit::textChanged,
            [=](const QString& filterText)
            {
                m_filterAdapter->SetFilterString(filterText.toUtf8().data());
            });
    }

    FilteredDPE::~FilteredDPE()
    {
        delete m_ui;
    }

    void FilteredDPE::SetAdapter(AZStd::shared_ptr<AZ::DocumentPropertyEditor::DocumentAdapter> sourceAdapter)
    {
        m_sourceAdapter = sourceAdapter;
        m_filterAdapter->SetSourceAdapter(m_sourceAdapter);
        m_ui->m_dpe->SetAdapter(m_filterAdapter);
    }

    DocumentPropertyEditor* FilteredDPE::GetDPE()
    {
        return m_ui->m_dpe;
    }

} // namespace AzToolsFramework
