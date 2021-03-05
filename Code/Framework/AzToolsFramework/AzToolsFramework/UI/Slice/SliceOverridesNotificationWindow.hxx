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

namespace Ui
{
    class NotificationWindow;
}

class QToolButton;

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
