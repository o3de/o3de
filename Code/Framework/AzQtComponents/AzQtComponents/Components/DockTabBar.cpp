/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>

#include <AzQtComponents/Components/DockBar.h>
#include <AzQtComponents/Components/DockBarButton.h>
#include <AzQtComponents/Components/DockMainWindow.h>
#include <AzQtComponents/Components/DockTabBar.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/Titlebar.h>

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QGraphicsOpacityEffect>
#include <QMenu>
#include <QMouseEvent>
#include <QToolButton>
#include <QStyleOptionTab>

// Constant for the width of the close button and its total offset (width + margin spacing)
static const int g_closeButtonWidth = 19;
static const int g_closeButtonOffset = g_closeButtonWidth + AzQtComponents::DockBar::ButtonsSpacing;
// Constant for the color of our tab indicator underlay
static const QColor g_tabIndicatorUnderlayColor(Qt::black);
// Constant for the opacity of our tab indicator underlay
static const qreal g_tabIndicatorUnderlayOpacity = 0.75;


namespace AzQtComponents
{
    /**
     * The close button is only present on the active tab, so return the close button offset for the
     * current index, otherwise none
     */
    int DockTabBar::closeButtonOffsetForIndex(const QStyleOptionTab* option)
    {
        return (option->state & QStyle::State_Selected) ? g_closeButtonOffset : 0;
    }

    /**
     * Create a dock tab widget that extends a QTabWidget with a custom DockTabBar to replace the default tab bar
     */
    DockTabBar::DockTabBar(QWidget* parent)
        : TabBar(parent)
        , m_tabIndicatorUnderlay(new QWidget(this))
        , m_leftButton(nullptr)
        , m_rightButton(nullptr)
        , m_contextMenu(nullptr)
        , m_closeTabMenuAction(nullptr)
        , m_closeTabGroupMenuAction(nullptr)
        , m_menuActionTabIndex(-1)
        , m_singleTabFillsWidth(false)
    {
        setFixedHeight(DockBar::Height);
        setMovable(true);
        SetUseMaxWidth(true);

        // Handle our close tab button clicks
        QObject::connect(this, &DockTabBar::tabCloseRequested, this, &DockTabBar::closeTab);

        // Handle when our current tab index changes
        QObject::connect(this, &TabBar::currentChanged, this, &DockTabBar::currentIndexChanged);

        // Our QTabBar base class has left/right indicator buttons for scrolling
        // through the tab header if all the tabs don't fit in the given space for
        // the widget, but they just float over the tabs, so we have added a
        // semi-transparent underlay that will be positioned below them so that
        // it looks better
        QPalette underlayPalette;
        underlayPalette.setColor(QPalette::Window, g_tabIndicatorUnderlayColor);
        m_tabIndicatorUnderlay->setAutoFillBackground(true);
        m_tabIndicatorUnderlay->setPalette(underlayPalette);
        QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(m_tabIndicatorUnderlay);
        effect->setOpacity(g_tabIndicatorUnderlayOpacity);
        m_tabIndicatorUnderlay->setGraphicsEffect(effect);

        // The QTabBar has two QToolButton children that are used as left/right
        // indicators to scroll across the tab header when the width is too short
        // to fit all of the tabs
        for (QToolButton* button : findChildren<QToolButton*>(QString(), Qt::FindDirectChildrenOnly))
        {
            // Grab references to each button for use later
            if (button->accessibleName() == TabBar::tr("Scroll Left"))
            {
                m_leftButton = button;
            }
            else
            {
                m_rightButton = button;
            }
        }
    }

    void DockTabBar::setIsShowingWindowControls(bool show)
    {
        m_isShowingWindowControls = show;
    }

    /**
     * Handle resizing appropriately when our parent tab widget is resized,
     * otherwise when there is only one tab it won't know to stretch to the
     * full width
     */
    QSize DockTabBar::sizeHint() const
    {
        if (m_singleTabFillsWidth && count() == 1)
        {
            return TabBar::tabSizeHint(0);
        }

        return TabBar::sizeHint();
    }

    void DockTabBar::setSingleTabFillsWidth(bool singleTabFillsWidth)
    {
        if (m_singleTabFillsWidth == singleTabFillsWidth)
        {
            return;
        }

        m_singleTabFillsWidth = singleTabFillsWidth;
        emit singleTabFillsWidthChanged(m_singleTabFillsWidth);
    }

    QString DockTabBar::tabText(int index) const
    {
        QString title = QTabBar::tabText(index);

        int titleWidth;
        return DockBar::GetTabTitleElided(title, titleWidth);
    }

    /**
     * Any time the tab layout changes (e.g. tabs are added/removed/resized or active tab changed),
     * we need to check if we need to add our tab indicator underlay, and
     * update which tab close button is visible
     */
    void DockTabBar::tabLayoutChange()
    {
        TabBar::tabLayoutChange();

        // Only the active tab's close button should be shown
        const ButtonPosition closeSide = (ButtonPosition)style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, nullptr, this);
        const int numTabs = count();
        const int activeTabIndex = currentIndex();
        for (int i = 0; i < numTabs; ++i)
        {
            if (auto button = tabButton(i, closeSide))
            {
                button->setVisible(i == activeTabIndex);
            }
        }

        // If the tab indicators are showing, then we need to show our underlay
        if (m_leftButton->isVisible())
        {
            // The underlay will take up the combined space behind the left and
            // right indicator buttons
            QRect total = m_leftButton->geometry();
            total = total.united(m_rightButton->geometry());
            m_tabIndicatorUnderlay->setGeometry(total);

            // The indicator buttons get raised when shown, so we need to stack
            // our underlay under the left button, which will place it under
            // both indicator buttons, and then show it
            m_tabIndicatorUnderlay->stackUnder(m_leftButton);
            m_tabIndicatorUnderlay->show();
        }
        else
        {
            m_tabIndicatorUnderlay->hide();
        }
    }

    void DockTabBar::tabInserted(int index)
    {
        auto closeButton = new DockBarButton(DockBarButton::CloseButton);
        connect(closeButton, &DockBarButton::clicked, this, [=] {
            int widgetIndex = tabAt(closeButton->pos());
            if (widgetIndex >= 0)
            {
                emit tabCloseRequested(widgetIndex);
            }
        });

        const ButtonPosition closeSide = (ButtonPosition) style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, nullptr, this);
        setTabButton(index, closeSide, closeButton);
    }

    /**
     * Handle the right-click context menu event by displaying our custom menu
     * with options to close/undock individual tabs or the entire tab group
     */
    void DockTabBar::contextMenuEvent(QContextMenuEvent* event)
    {
        // Figure out the index of the tab the event was triggered on, or use
        // the currently active tab if the event was triggered in the header
        // dead zone
        int index = tabAt(event->pos());
        if (index == -1 && !m_isShowingWindowControls)
        {
            index = currentIndex();
        }
        m_menuActionTabIndex = index;

        // Need to create our context menu/actions if this is the first time
        // it has been invoked
        const bool isValidIndex = (index >= 0);
        if (!m_contextMenu)
        {
            m_contextMenu = new QMenu(this);
            const QString tabName = isValidIndex ? QTabBar::tabText(index) : QString();
            // Action to close the specified tab, and leave the text blank since
            // it will be dynamically set using the title of the specified tab
            m_closeTabMenuAction = m_contextMenu->addAction(tr("Close %1").arg(tabName));
            QObject::connect(m_closeTabMenuAction, &QAction::triggered, this, [this]() { emit closeTab(m_menuActionTabIndex); });

            // Action to close all of the tabs in our tab widget
            m_closeTabGroupMenuAction = m_contextMenu->addAction(tr("Close Tab Group"));
            QObject::connect(m_closeTabGroupMenuAction, &QAction::triggered, this, &DockTabBar::closeTabGroup);

            // Separate the close actions from the undock actions
            m_contextMenu->addSeparator();

            // Action to undock the specified tab, and leave the text blank since
            // it will be dynamically set using the title of the specified tab
            m_undockTabMenuAction = m_contextMenu->addAction(tr("Undock %1").arg(tabName));
            QObject::connect(m_undockTabMenuAction, &QAction::triggered, this, [this]() { emit undockTab(m_menuActionTabIndex); });

            // Action to undock the entire tab widget
            m_undockTabGroupMenuAction = m_contextMenu->addAction(tr("Undock Tab Group"));
            QObject::connect(m_undockTabGroupMenuAction, &QAction::triggered, this ,[this]() { emit undockTab(-1); });
        }

        if (isValidIndex)
        {
            // Only enable the close/undock group actions if we have more than one
            // tab in our tab widget
            bool enableGroupActions = (count() > 1);
            m_closeTabGroupMenuAction->setEnabled(enableGroupActions);

            // Don't enable the undock action if this dock widget is the only pane
            // in a floating window or if it isn't docked in one of our dock main windows
            QWidget* tabWidget = parentWidget();
            bool enableUndock = true;
            if (tabWidget)
            {
                // The main case is when we have a tab bar for a tab widget
                QWidget* tabWidgetParent = tabWidget->parentWidget();
                StyledDockWidget* dockWidgetContainer = qobject_cast<StyledDockWidget*>(tabWidgetParent);
                if (!dockWidgetContainer)
                {
                    // The other case is when this tab bar is being used for a solo dock widget by the TitleBar
                    // so that it looks like a tab, so we need to look one level up
                    dockWidgetContainer = qobject_cast<StyledDockWidget*>(tabWidgetParent->parentWidget());
                }

                if (dockWidgetContainer)
                {
                    DockMainWindow* dockMainWindow = qobject_cast<DockMainWindow*>(dockWidgetContainer->parentWidget());

                    enableUndock = dockMainWindow && !dockWidgetContainer->isSingleFloatingChild();
                }
            }
            m_undockTabGroupMenuAction->setEnabled(enableGroupActions && enableUndock);

            // Enable the undock action if there are multiple tabs or if this isn't
            // a single tab in a floating window
            m_undockTabMenuAction->setEnabled(enableGroupActions || enableUndock);

            // Show the context menu
            m_contextMenu->exec(event->globalPos());
        }
        else
        {
            // Show Window context menu

            // The Floating Window structure is fixed, so we should always get the parent. If we don't, bail out.
            if (!parent() || !parent()->parent() || !parent()->parent()->parent())
            {
                AZ_Warning("DockTabBar", false,
                    "Could not access the parent floating window to trigger its context menu - invalid floating window structure?");
                return;
            }

            auto parentFloatingWindow = qobject_cast<StyledDockWidget*>(parent()->parent()->parent()->parent());
            if (parentFloatingWindow)
            {
                auto parentFloatingWindowTitleBar = qobject_cast<AzQtComponents::TitleBar*>(parentFloatingWindow->customTitleBar());
                if (parentFloatingWindowTitleBar)
                {
                    QContextMenuEvent contextMenuEvent(QContextMenuEvent::Reason::Mouse, event->pos(), event->globalPos());
                    QApplication::sendEvent(parentFloatingWindowTitleBar, &contextMenuEvent);
                }
            }
        }
    }

    /**
     * Close all of the tabs in our tab widget
     */
    void DockTabBar::closeTabGroup()
    {
        // Close each of the tabs using our signal trigger so they are cleaned
        // up properly
        int numTabs = count();
        for (int i = 0; i < numTabs; ++i)
        {
            emit closeTab(0);
        }
    }

    /**
     * When our tab index changes, we need to force a resize event to trigger a layout change, since the tabSizeHint needs
     * to be updated because we only show the close button on the active tab
     */
    void DockTabBar::currentIndexChanged(int current)
    {
        Q_UNUSED(current);

        resizeEvent(nullptr);
    }

    /**
     * Override the mouse press event handler to fix a Qt issue where the QTabBar
     * doesn't ensure that it's the left mouse button that has been pressed
     * early enough, even though it properly checks for it first in its mouse
     * release event handler
     */
    void DockTabBar::mousePressEvent(QMouseEvent* event)
    {
        if (event->button() != Qt::LeftButton)
        {
            event->ignore();
            return;
        }

        TabBar::mousePressEvent(event);
    }

    /**
     * Send a dummy MouseButtonRelease event to the QTabBar to ensure that the tab move animations
     * get triggered when a tab is dragged out of the tab bar.
     */
    void DockTabBar::finishDrag()
    {
        QMouseEvent event(QEvent::MouseButtonRelease, {0.0f, 0.0f}, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        mouseReleaseEvent(&event);
    }

} // namespace AzQtComponents

#include "Components/moc_DockTabBar.cpp"
