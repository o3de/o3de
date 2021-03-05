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

