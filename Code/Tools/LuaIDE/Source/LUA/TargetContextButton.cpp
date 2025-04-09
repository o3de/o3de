/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TargetContextButton.hxx"
#include <Source/LUA/moc_TargetContextButton.cpp>
#include <Source/LUA/LUAEditorContextMessages.h>
#include <Source/LUA/LUAEditorDebuggerMessages.h>
#include <Source/LUA/LUATargetContextTrackerMessages.h>

#include <QMenu>

namespace LUA
{
    TargetContextButton::TargetContextButton(QWidget* pParent)
        : QPushButton(pParent)
    {
        LUAEditor::Context_ControlManagement::Handler::BusConnect();

        AZStd::string context("Default");
        LUAEditor::LUATargetContextRequestMessages::Bus::Broadcast(
            &LUAEditor::LUATargetContextRequestMessages::Bus::Events::SetCurrentTargetContext, context);
        this->setText("Context: Default");

        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sizePolicy().hasHeightForWidth());
        setSizePolicy(sizePolicy1);
        setMinimumSize(QSize(128, 24));

        this->setToolTip(tr("Click to change context"));
        connect(this, SIGNAL(clicked()), this, SLOT(DoPopup()));
    }

    TargetContextButton::~TargetContextButton()
    {
        LUAEditor::Context_ControlManagement::Handler::BusDisconnect();
    }

    void TargetContextButton::DoPopup()
    {
        QMenu menu;

        AZStd::vector<AZStd::string> contexts;
        LUAEditor::LUATargetContextRequestMessages::Bus::BroadcastResult(
            contexts, &LUAEditor::LUATargetContextRequestMessages::Bus::Events::RequestTargetContexts);

        for (AZStd::vector<AZStd::string>::const_iterator it = contexts.begin(); it != contexts.end(); ++it)
        {
            QAction* targetAction = new QAction((*it).c_str(), this);
            targetAction->setProperty("context", (*it).c_str());
            menu.addAction(targetAction);
        }

        QAction* resultAction = menu.exec(QCursor::pos());

        if (resultAction)
        {
            AZStd::string context = resultAction->property("context").toString().toUtf8().data();
            this->setText("Context: None"); // prepare for failure
            LUAEditor::LUATargetContextRequestMessages::Bus::Broadcast(
                &LUAEditor::LUATargetContextRequestMessages::Bus::Events::SetCurrentTargetContext, context);
        }
    }

    void TargetContextButton::OnTargetContextPrepared(AZStd::string& contextName)
    {
        QString qstr = QString("Context: %1").arg(contextName.c_str());
        this->setText(qstr); // plan for success
    }

    TargetContextButtonAction::TargetContextButtonAction(QObject* pParent)
        : QWidgetAction(pParent)
    {
    }

    QWidget* TargetContextButtonAction::createWidget(QWidget* pParent)
    {
        return aznew  TargetContextButton(pParent);
    }
}
