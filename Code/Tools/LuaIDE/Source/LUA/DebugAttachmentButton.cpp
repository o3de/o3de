/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DebugAttachmentButton.hxx"
#include <Source/LUA/moc_DebugAttachmentButton.cpp>
#include <Source/LUA/LUAContextControlMessages.h>
#include <Source/LUA/LUAEditorContextMessages.h>

#include <QPainter>
#include <QStyleOptionButton>

namespace LUAEditor
{
    DebugAttachmentButton::DebugAttachmentButton(QWidget* pParent)
        : QPushButton(pParent)
    {

        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sizePolicy().hasHeightForWidth());
        setSizePolicy(sizePolicy1);

        // default state to disconnected and unattached
        // change state on later message updates
        OnDebuggerDetached();

        connect(this, SIGNAL(clicked()), this, SLOT(OnClicked()));
        Context_ControlManagement::Handler::BusConnect();
    }

    DebugAttachmentButton::~DebugAttachmentButton()
    {
        Context_ControlManagement::Handler::BusDisconnect();
    }

    void DebugAttachmentButton::paintEvent(QPaintEvent* /* event */)
    {
        QPainter painter(this);

        QStyleOptionButton option;
        option.initFrom(this);
        option.features = QStyleOptionButton::None;
        option.text = text();
        option.icon = icon();
        option.iconSize = iconSize();

        style()->drawControl(QStyle::CE_PushButton, &option, &painter, this);
    }

    void DebugAttachmentButton::OnClicked()
    {
        switch (m_State)
        {
        case DAS_ATTACHED:
            Context_DebuggerManagement::Bus::Broadcast(&Context_DebuggerManagement::Bus::Events::RequestDetachDebugger);
            break;
        case DAS_UNATTACHED:
        case DAS_REFUSED:
        {
            Context_DebuggerManagement::Bus::Broadcast(&Context_DebuggerManagement::Bus::Events::RequestAttachDebugger);
        }
        break;
        }
    }

    void DebugAttachmentButton::OnDebuggerAttached()
    {
        this->setToolTip(tr("Click to detach from debugging"));
        UpdateStatus(DAS_ATTACHED);
    }
    void DebugAttachmentButton::OnDebuggerRefused()
    {
        this->setToolTip(tr("Target refused debug request.  Click here to retry attaching"));
        UpdateStatus(DAS_REFUSED);
    }
    void DebugAttachmentButton::OnDebuggerDetached()
    {
        this->setToolTip(tr("Click to attach for debugging"));
        UpdateStatus(DAS_UNATTACHED);
    }

    void DebugAttachmentButton::UpdateStatus(DebugAttachmentState newState)
    {
        m_State = newState;

        switch (newState)
        {
        case DAS_ATTACHED:
            this->setIcon(QIcon(":/debug/debugger_connected"));
            this->setText("Debugging: ON");
            break;
        case DAS_UNATTACHED:
            this->setIcon(QIcon(":/debug/debugger_disconnected"));
            this->setText("Debugging: OFF");
            break;
        case DAS_REFUSED:
            this->setIcon(QIcon(":/general/target_none"));
            this->setText("Debugging: Refused");
            break;
        }
    }

    //-----------------------------------------------------------------------
    DebugAttachmentButtonAction::DebugAttachmentButtonAction(QObject* pParent)
        : QWidgetAction(pParent)
    {
    }

    QWidget* DebugAttachmentButtonAction::createWidget(QWidget* pParent)
    {
        return aznew DebugAttachmentButton(pParent);
    }
}
