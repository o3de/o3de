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

#include "AzToolsFramework_precompiled.h"

#include <AzToolsFramework/SourceControl/QtSourceControlNotificationHandler.h>
#include <AzCore/std/string/string.h>

#include <QMessageBox>
#include <QMetaObject>

namespace AzToolsFramework
{

    QtSourceControlNotificationHandler::QtSourceControlNotificationHandler(QWidget* pParent)
        : QObject(pParent)
    {
    }
        
    QtSourceControlNotificationHandler::~QtSourceControlNotificationHandler()
    {
        Shutdown();
    }

    void QtSourceControlNotificationHandler::Init()
    {
        SourceControlNotificationBus::Handler::BusConnect();
    }

    void QtSourceControlNotificationHandler::Shutdown()
    {
        SourceControlNotificationBus::Handler::BusDisconnect();
    }

    void QtSourceControlNotificationHandler::RequestTrust(const char* fingerprint)
    {
        QString message = QString("%1\n\n%2\n\n%3")
            .arg("The fingerprint for the key sent to your client is:")
            .arg(fingerprint)
            .arg("Establish Trust?");

        auto azFingerprint = AZStd::string(fingerprint);
        auto userAnswer = QMessageBox::question(qobject_cast<QWidget*>(parent()), "Establish Trust?", message,
            QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No);

        using SCRequestBus = AzToolsFramework::SourceControlConnectionRequestBus;
        SCRequestBus::Broadcast(&SCRequestBus::Events::EnableTrust, userAnswer == QMessageBox::Yes, azFingerprint);

        if (userAnswer == QMessageBox::No)
        {
            SCRequestBus::Broadcast(&SCRequestBus::Events::EnableSourceControl, false);
        }
    }

} // namespace AzToolsFramework

#include "SourceControl/moc_QtSourceControlNotificationHandler.cpp"
