/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "EMStudioConfig.h"
#include <MCore/Source/MemoryManager.h>
#include <QToolButton>
#include <QWidget>
#include <QTimer>
#include <QLabel>
#endif


namespace EMStudio
{
    class EMSTUDIO_API NotificationWindow
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(NotificationWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        enum EType
        {
            TYPE_ERROR      = 0,
            TYPE_WARNING    = 1,
            TYPE_SUCCESS    = 2
        };

    public:
        NotificationWindow(QWidget* parent, EType type, const QString& Message);
        ~NotificationWindow();

    protected:
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;

    private slots:
        void IconPressed();
        void TimerTimeOut();
        void OpacityChanged(qreal opacity);
        void FadeOutFinished();

    private:
        QLabel*         mMessageLabel;
        QToolButton*    mIcon;
        QTimer*         mTimer;
        int             mOpacity;
    };
} // namespace EMStudio
