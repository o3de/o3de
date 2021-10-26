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
#include <AzQtComponents/Components/ToastNotificationConfiguration.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <QEvent>
#include <QDialog>
#include <QMouseEvent>
#include <QTimer>
#endif

namespace Ui
{
    class ToastNotification;
}

QT_FORWARD_DECLARE_CLASS(QPropertyAnimation)

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API ToastNotification
        : public QDialog
    {
        Q_OBJECT
    public:    
        AZ_CLASS_ALLOCATOR(ToastNotification, AZ::SystemAllocator, 0);
        
        ToastNotification(QWidget* parent, const ToastConfiguration& toastConfiguration);
        virtual ~ToastNotification();

        // Shows the toast notification relative to the current cursor.        
        void ShowToastAtCursor();
        
        // Aligns the toast notification so that the specified anchor point on the notification lies on the specified screen position.
        // i.e. anchor point of 0,0 will align the top left position of the dialog with the screen position
        //      anchor point of 1,1 will align the bottom right position of the dialog with the screen position
        void ShowToastAtPoint(const QPoint& screenPosition, const QPointF& anchorPoint);

        void UpdatePosition(const QPoint& screenPosition, const QPointF& anchorPoint);

        bool IsDuplicate(const ToastConfiguration& toastConfiguration);
        
        // QDialog
        void showEvent(QShowEvent* showEvent) override;
        void hideEvent(QHideEvent* hideEvent) override;
        void mousePressEvent(QMouseEvent* mouseEvent) override;
        bool eventFilter(QObject* object, QEvent* event) override;
        
        void paintEvent(QPaintEvent* event) override;

    public slots:
        void StartTimer();
        void FadeOut();

    signals:    
        void ToastNotificationHidden();
        void ToastNotificationInteraction();
        
    private:
        QPropertyAnimation* m_fadeAnimation;
        ToastConfiguration m_configuration;
        bool   m_closeOnClick;
        QTimer m_lifeSpan;
        uint32_t m_borderRadius = 0;

        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        AZStd::chrono::milliseconds m_fadeDuration;
        AZStd::unique_ptr<Ui::ToastNotification> m_ui;        
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    };
} // namespace AzQtComponents
