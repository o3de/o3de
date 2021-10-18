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
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#include <AzQtComponents/Components/DockBarButton.h>

#include <QMap>
#include <QWidget>
#endif


class QDockWidget;
class QMouseEvent;
class QVBoxLayout;
class QStackedWidget;

namespace AzQtComponents
{
    class DockTabBar;
    class TitleBar;
    typedef QList<DockBarButton::WindowDecorationButton> WindowDecorationButtons;

    class AZ_QT_COMPONENTS_API DockTabWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        explicit DockTabWidget(QWidget* mainEditorWindow, QWidget* parent = nullptr);
        void setButtons(WindowDecorationButtons buttons);
        int addTab(QDockWidget* page);
        void removeTab(int index);
        void removeTab(QDockWidget* page);
        bool closeTabs();
        void moveTab(int from, int to);
        void mousePressEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void finishDrag();
        QString tabText(int index) const;
        void setCurrentIndex(int index);
        void setCurrentWidget(QWidget* widget);
        int indexOf(QWidget* widget);
        int count();
        int currentIndex();
        DockTabBar* tabBar();
        QWidget* widget(int index);

        static bool IsTabbed(QDockWidget* dockWidget);
        static DockTabWidget* ParentTabWidget(QDockWidget* dockWidget);

    Q_SIGNALS:
        void tabIndexPressed(int index);
        void tabCountChanged(int count);
        void tabWidgetInserted(QWidget* widget);
        void undockTab(int index);
        void tabBarDoubleClicked();
        void currentChanged(int index);

    protected:
        void closeEvent(QCloseEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;
        // void tabInserted(int index);
        // void tabRemoved(int index);

    protected Q_SLOTS:
        bool handleTabCloseRequested(int index);
        void handleTabAboutToClose();
        void handleTabIndexPressed(int index);

    private Q_SLOTS:
        void onTabMoved(int from, int to);
        void onRemovedTab(int index);
        void onShowTab(int index);

    private:
        QVBoxLayout* m_mainLayout;
        QStackedWidget* m_stack;

        TitleBar* m_tilteBar;
        QWidget* m_mainEditorWindow; 
        DockTabBar* m_tabBar;

        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        QMap<QDockWidget*, QMetaObject::Connection> m_titleBarChangedConnections;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    };
} // namespace AzQtComponents
