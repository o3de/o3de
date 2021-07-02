/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QWidget>
#endif

class QIcon;
class QToolButton;
class QEvent;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API ToolButtonWithWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        QSize sizeHint() const override;
        void setIcon(const QIcon&);
        void setIconSize(const QSize &iconSize);
        QToolButton* button() const;

    protected:
        explicit ToolButtonWithWidget(QWidget* widget, QWidget* parent = nullptr);
        bool event(QEvent *event) override;
        QWidget* widget();

Q_SIGNALS:
        void clicked();

    private:
        void connectToParentToolBar();

        QSize m_iconSize = QSize(16, 16);
        QToolButton* const m_button;
        QWidget* const m_widget;
    };
} // namespace AzQtComponents

