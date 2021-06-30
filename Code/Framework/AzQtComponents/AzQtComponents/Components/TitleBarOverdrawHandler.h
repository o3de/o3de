/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QObject>
#include <QMargins>

class QApplication;
class QWidget;

Q_DECLARE_METATYPE(QMargins)

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API TitleBarOverdrawHandler :
        public QObject
    {
    public:
        explicit TitleBarOverdrawHandler(QObject* parent);
        ~TitleBarOverdrawHandler() override;

        virtual void polish(QWidget* /*widget*/) {}
        virtual void addTitleBarOverdrawWidget(QWidget* /*widget*/) {}

        static TitleBarOverdrawHandler* getInstance();

        // Must be defined by the platform specific implementation
        static TitleBarOverdrawHandler* createHandler(QApplication* application, QObject* parent);
    };

} // namespace AzQtComponents

