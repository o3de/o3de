/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QEvent>
#include <QDialog>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QTimer>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/Editor/EditorTypes.h>
#endif

namespace Ui
{
    class ToastNotification;
}

namespace GraphCanvas
{
    class ToastNotification
        : public QDialog
    {
        Q_OBJECT
    public:    
        AZ_CLASS_ALLOCATOR(ToastNotification, AZ::SystemAllocator, 0);
        
        ToastNotification(QWidget* parent, const ToastConfiguration& configuration);
        virtual ~ToastNotification();

        ToastId GetToastId() const;
        
        // Shows the toast notification relative to the current cursor.        
        void ShowToastAtCursor();
        
        // Aligns the toast notification so that the specified anchor point on the notification lies on the specified screen position.
        // i.e. anchor point of 0,0 will align the top left position of the dialog with the screen position
        //      anchor point of 1,1 will align the bottom right position of the dialog with the screen position
        void ShowToastAtPoint(const QPoint& screenPosition, const QPointF& anchorPoint);

        void UpdatePosition(const QPoint& screenPosition, const QPointF& anchorPoint);
        
        // QDialog
        void showEvent(QShowEvent* showEvent) override;
        void hideEvent(QHideEvent* hideEvent) override;

        void mousePressEvent(QMouseEvent* mouseEvent) override;

        bool eventFilter(QObject* object, QEvent* event) override;
        ////
        
    public slots:

        void StartTimer();
        void FadeOut();

    signals:    
        void ToastNotificationShown();
        void ToastNotificationHidden();
        
    private:

        QPropertyAnimation* m_fadeAnimation;
        AZStd::chrono::milliseconds m_fadeDuration;

        ToastId m_toastId;

        bool   m_closeOnClick;
        QTimer m_lifeSpan;
        AZStd::unique_ptr<Ui::ToastNotification> m_ui;        
    };
}
