/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
