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
