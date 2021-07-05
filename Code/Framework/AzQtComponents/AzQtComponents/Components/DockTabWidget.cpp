/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/DockTabWidget.h>
#include <AzQtComponents/Components/DockTabBar.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/RepolishMinimizer.h>

#include <QContextMenuEvent>
#include <QDockWidget>
#include <QStackedWidget>
#include <QString>


namespace AzQtComponents
{
    /**
     * Create a dock tab widget that extends a QTabWidget with a custom DockTabBar to replace the default tab bar
     */
    DockTabWidget::DockTabWidget(QWidget* mainEditorWindow, QWidget* parent)
        : TabWidget(parent)
        , m_tabBar(new DockTabBar)
        , m_mainEditorWindow(mainEditorWindow)
    {
        // Replace the default tab bar with our custom DockTabBar to override the styling and docking behaviors.
        // Setting the custom tab bar parents it to ourself, so it will get cleaned up whenever our DockTabWidget is destroyed.
        setCustomTabBar(m_tabBar);

        // Listen for selected tab changes
        QObject::connect(m_tabBar, &DockTabBar::tabBarClicked, this, &DockTabWidget::handleTabIndexPressed);

        // Listen for close requests from our tabs
        QObject::connect(m_tabBar, &DockTabBar::closeTab, this, &DockTabWidget::handleTabCloseRequested);

        // Forward on undock requests from our tabs
        QObject::connect(m_tabBar, &DockTabBar::undockTab, this, &DockTabWidget::undockTab);

        // Enable spacing of the overflow button to allow dragging even when the TabBar gets crowded
        setOverflowButtonSpacing(true);
    }

    /**
     * Small wrapper around the QTabWidget add tab so we can strip off the title bar widget automatically
     */
    int DockTabWidget::addTab(QDockWidget* page)
    {
        if (!page)
        {
            return -1;
        }

        // Set an empty QWidget as the custom title bar to hide it, since our tab widget will drive it's own custom tab bar
        // that will replace it (the empty QWidget is parented to the dock widget, so it will be cleaned up whenever the dock widget is deleted).
        // We can't just set a nullptr because the QDockWidget will install a default title bar instead of leaving it empty, which is what we want.
        page->setTitleBarWidget(new QWidget());

        // Let the QTabWidget handle the rest
        AzQtComponents::RepolishMinimizer minimizer;
        int tab = TabWidget::addTab(page, page->windowTitle());

        // If a tabbed dock widget is about to close, we need to remove it from
        // our tab widget ourselves, otherwise it won't know to recreate its
        // createCustomTitleBar() that we set to empty when adding to a tab widget
        auto dockWidget = qobject_cast<AzQtComponents::StyledDockWidget*>(page);
        if (dockWidget)
        {
            QObject::connect(dockWidget, &StyledDockWidget::aboutToClose, this, &DockTabWidget::handleTabAboutToClose);
        }

        // Make sure that changes to the window title get reflected by the tab too
        m_titleBarChangedConnections[page] = connect(page, &QWidget::windowTitleChanged, this, [this, page]() {
            // have to find this widget in the list, since the index might have changed
            int tabIndex = indexOf(page);
            if (tabIndex != -1)
            {
                setTabText(tabIndex, page->windowTitle());
            }
        });

        return tab;
    }

    /**
     * Small wrapper around the QTabWidget remove tab so we can put the custom
     * title bar back on the dock widget since we stripped it off when adding the tab
     * and unparent it
     */
    void DockTabWidget::removeTab(int index)
    {
        AzQtComponents::StyledDockWidget* dockWidget = qobject_cast<AzQtComponents::StyledDockWidget*>(widget(index));
        if (dockWidget)
        {
            // Stop listening to title bar changed events
            if (m_titleBarChangedConnections.find(dockWidget) != m_titleBarChangedConnections.end())
            {
                QObject::disconnect(m_titleBarChangedConnections[dockWidget]);
                m_titleBarChangedConnections.remove(dockWidget);
            }

            // Restore the custom title bar so if the user re-opens that view,
            // the custom title bar will still be there
            dockWidget->createCustomTitleBar();

            // We also need to unparent it before we remove it from
            // the tab widget, or else the tab widget may try to delete it during cleanup
            // if it was the last remaining tab, which results in the tab widget being deleted.
            // Unparenting the dock widget will trigger the QTabWidget to call removeTab
            // itself, so we shouldn't call it here, or else it would end up removing
            // the index twice, which could inadvertently remove two different widgets.
            // We accomplish the unparent by re-parenting the dock widget to the
            // main editor window, which will allow it to be restored properly later.
            AzQtComponents::RepolishMinimizer minimizer;
            dockWidget->setParent(m_mainEditorWindow);
        }
    }

    /**
     * Overloaded function to be able to remove a tab by passing the dock widget
     */
    void DockTabWidget::removeTab(QDockWidget* page)
    {
        int index = indexOf(page);
        if (index != -1)
        {
            removeTab(index);
        }
    }

    /**
     * Attempt to close all of the tabs and return whether or not the tabs were
     * closed successfully
     */
    bool DockTabWidget::closeTabs()
    {
        int numTabs = count();
        for (int i = 0; i < numTabs; ++i)
        {
            if (!handleTabCloseRequested(0))
            {
                return false;
            }
        }

        return true;
    }

    /**
     * Expose QTabBar API to reorder existing tabs
     */
    void DockTabWidget::moveTab(int from, int to)
    {
        m_tabBar->moveTab(from, to);
    }

    /**
     * Handle the close event for our tab widget by trying to close all of the tabs
     */
    void DockTabWidget::closeEvent(QCloseEvent* event)
    {
        // If any of the tabs couldn't be closed, then ignore our close event
        if (!closeTabs())
        {
            event->ignore();
            return;
        }

        TabWidget::closeEvent(event);
    }

    /**
     * Handle passing our right-click context menu event to our custom tab bar
     * when necessary
     */
    void DockTabWidget::contextMenuEvent(QContextMenuEvent* event)
    {
        // The custom tab bar doesn't take up the full width of our tab widget,
        // so we need to forward on any right-click events in this dead zone
        // on the right of the tab bar to it so it can display the appropriate
        // context menu
        if (event->pos().y() <= m_tabBar->height())
        {
            m_tabBar->contextMenuEvent(event);
        }
    }

    /**
     * Emit a signal with a reference to the widget that was inserted
     */
    void DockTabWidget::tabInserted(int index)
    {
        TabWidget::tabInserted(index);
        emit tabWidgetInserted(widget(index));
    }

    /**
     * Emit our tab count changed signal whenever a tab is removed (we use this elsewhere to handle tearing down the tab widget when no tabs are left)
     */
    void DockTabWidget::tabRemoved(int index)
    {
        TabWidget::tabRemoved(index);
        emit tabCountChanged(count());
    }

    /**
     * Handle closing tabs when requested from our tab bar close button
     */
    bool DockTabWidget::handleTabCloseRequested(int index)
    {
        AzQtComponents::StyledDockWidget* dockWidget = qobject_cast<AzQtComponents::StyledDockWidget*>(widget(index));
        if (dockWidget)
        {
            // Send the close event to the widget, so it has the opportunity to reject or save its state.
            // If the DeleteOnClose attribute is set, then the tab will be removed automatically if the
            // close is successful when the widget is deleted. Otherwise, it will get removed since we
            // are listening to the aboutToClose signal from our tabbed dock widgets.
            return dockWidget->close();
        }

        return false;
    }

    void DockTabWidget::handleTabAboutToClose()
    {
        auto dockWidget = qobject_cast<AzQtComponents::StyledDockWidget*>(sender());
        if (dockWidget)
        {
            removeTab(dockWidget);
        }
    }

    void DockTabWidget::handleTabIndexPressed(int index)
    {
        // Give the dock widget for the selected tab focus, since the user is explicitly switching
        // to that tab
        QDockWidget* dockWidget = qobject_cast<QDockWidget*>(widget(index));
        if (dockWidget)
        {
            dockWidget->setFocus();
        }

        // Pass along our signal that the tab index was pressed
        Q_EMIT tabIndexPressed(index);
    }

    /**
     * Handle clicks in the tab bar. Will only pick up clicks in the black, unoccupied area.
     */
    void DockTabWidget::mousePressEvent(QMouseEvent *event)
    {
        if (event->button() == Qt::MouseButton::LeftButton && event->pos().y() <= m_tabBar->height())
        {
            emit tabIndexPressed(-1);
        }
        else
        {
            TabWidget::mousePressEvent(event);
        }
    }

    /**
     * Pass along the mouse move events to our custom tab bar
     */
    void DockTabWidget::mouseMoveEvent(QMouseEvent* event)
    {
        m_tabBar->mouseMoveEvent(event);
    }

    /**
     * Pass along the mouse release events to our custom tab bar
     */
    void DockTabWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        m_tabBar->mouseReleaseEvent(event);
    }

    void DockTabWidget::mouseDoubleClickEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::MouseButton::LeftButton && event->pos().y() <= m_tabBar->height())
        {
            emit tabBarDoubleClicked();
        }
        else
        {
            TabWidget::mouseDoubleClickEvent(event);
        }
    }

    /**
     * Handle clearing the internal drag state of our custom tab bar
     */
    void DockTabWidget::finishDrag()
    {
        m_tabBar->finishDrag();
    }

    /**
     * Check if this dock widget is tabbed in our custom dock tab widget
     */
    bool DockTabWidget::IsTabbed(QDockWidget* dockWidget)
    {
        // If our dock widget is tabbed, it will have a valid tab widget parent
        return ParentTabWidget(dockWidget);
    }

    /**
     * Return the tab widget holding this dock widget if it is a tab, otherwise nullptr
     */
    AzQtComponents::DockTabWidget* DockTabWidget::ParentTabWidget(QDockWidget* dockWidget)
    {
        if (dockWidget)
        {
            // If our dock widget is tabbed, it will be parented to a QStackedWidget that is parented to
            // our dock tab widget
            QStackedWidget* stackedWidget = qobject_cast<QStackedWidget*>(dockWidget->parentWidget());
            if (stackedWidget)
            {
                AzQtComponents::DockTabWidget* tabWidget = qobject_cast<AzQtComponents::DockTabWidget*>(stackedWidget->parentWidget());
                return tabWidget;
            }
        }

        return nullptr;
    }

} // namespace AzQtComponents

#include "Components/moc_DockTabWidget.cpp"
