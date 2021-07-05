/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
