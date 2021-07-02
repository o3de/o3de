/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EndpointEntryView.h"
#include "Qt/ui_EndpointEntryView.h"
#include <EndpointManager.h>
#include <QToolButton>

namespace News
{

    const char* EndpointEntryView::SELECTED_CSS =
        "background-color: rgb(60, 100, 60);\ncolor: white;";
    const char* EndpointEntryView::UNSELECTED_CSS =
        "background-color: rgb(60, 60, 60);\ncolor: white;";

    EndpointEntryView::EndpointEntryView(QWidget* parent, Endpoint* pEndpoint)
        : QWidget(parent)
        , m_ui(new Ui::EndpointEntryViewWidget)
        , m_pEndpoint(pEndpoint)
    {
        m_ui->setupUi(this);

        m_ui->labelName->setText(pEndpoint->GetName());

        connect(m_ui->labelName, &AzQtComponents::ExtendedLabel::clicked, this, &EndpointEntryView::selectSlot);
        connect(m_ui->buttonDelete, &QToolButton::clicked, this, &EndpointEntryView::deleteSlot);
    }

    EndpointEntryView::~EndpointEntryView() {}

    void EndpointEntryView::SetSelected(bool selected)
    {
        setStyleSheet(selected ? SELECTED_CSS : UNSELECTED_CSS);
    }

    Endpoint* EndpointEntryView::GetEndpoint() const
    {
        return m_pEndpoint;
    }

    void EndpointEntryView::selectSlot()
    {
        emit selectSignal(this);
    }

    void EndpointEntryView::deleteSlot()
    {
        emit deleteSignal(this);
    }

#include "Qt/moc_EndpointEntryView.cpp"

}
