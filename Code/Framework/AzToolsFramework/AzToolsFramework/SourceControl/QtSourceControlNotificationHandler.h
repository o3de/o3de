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
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <QString>
#include <QObject>
#endif

namespace AzToolsFramework
{
    class QtSourceControlNotificationHandler
        : public QObject
        , private SourceControlNotificationBus::Handler
    {
        Q_OBJECT
    public:
        explicit QtSourceControlNotificationHandler(QWidget* pParent);
        virtual ~QtSourceControlNotificationHandler();

        void Init();
        void Shutdown();

    private:
        // AzToolsFramework::SourceControlNotificationBus::Handler
        void RequestTrust(const char* fingerprint) override;
    };
} // namespace AzToolsFramework
