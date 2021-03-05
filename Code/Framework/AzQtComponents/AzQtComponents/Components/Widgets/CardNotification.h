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
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QFrame>
#include <QString>
#endif

class QVBoxLayout;
class QPushButton;
class QVBoxLayout;
class QIcon;

namespace AzQtComponents
{
    //! Notification class for Card widgets.
    //! Displays a message and allows for widgets to be added to handle error cases and solve requirements.
    class AZ_QT_COMPONENTS_API CardNotification
        : public QFrame
    {
        Q_OBJECT
    public:
        CardNotification(QWidget* parent, const QString& title, const QIcon& icon, const QSize = {24, 24});

        //! Appends a widget to the notification frame.
        //! The widget will be parented to the CardNotification.
        void addFeature(QWidget* feature);

        //! Creates a QPushButton and adds it to the notification frame.
        //! @param buttonText The text to display on the newly created button.
        QPushButton* addButtonFeature(const QString& buttonText);

    private:
        QVBoxLayout* m_featureLayout = nullptr;
    };
} // namespace AzQtComponents
