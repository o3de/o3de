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

#include <QWidget>
#endif

class QToolButton;

namespace AzQtComponents
{
    //! Displays custom buttons on the right side of the tab bar widget.
    class AZ_QT_COMPONENTS_API TabWidgetActionToolBar
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit TabWidgetActionToolBar(QWidget* parent = nullptr);

    signals:
        //! Triggered when the actions are changed.
        void actionsChanged();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        friend class TabWidget;
        friend class TabWidgetActionToolBarContainer;

        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        QHash<QAction*, QToolButton*> m_actionButtons;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

        void removeWidgetFromLayout(QWidget* widget);
    };

} // namespace AzQtComponents
