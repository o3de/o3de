/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QtWidgets/QWidget>

namespace Ui
{
    class NotificationWindow;
}

class QToolButton;
class QLabel;
class QTimer;

namespace AzToolsFramework
{
    /**
    * This class defines the notification window widget
    */
    class SliceOverridesNotificationWindow
        : public QWidget
    {
        Q_OBJECT

    public:
        enum class EType
        {
            TypeError,
            TypeSuccess
        };

    Q_SIGNALS:
        void RemoveNotificationWindow(SliceOverridesNotificationWindow* window);

    public:
        SliceOverridesNotificationWindow(QWidget* parent, EType type, const QString& Message);
        ~SliceOverridesNotificationWindow();

    protected:
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;

    private slots:
        void IconPressed();
        void TimerTimeOut();
        void OpacityChanged(qreal newOpacity);
        void FadeOutFinished();

    private:
        QLabel*         m_messageLabel;
        QToolButton*    m_icon;
        QTimer*         m_timer;
        float          m_opacity;
        QScopedPointer<Ui::NotificationWindow> m_ui;
    };
} // namespace AzToolsFramework
