/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <cmath>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Trace.h>

#include <AzQtComponents/Components/DockTabBar.h>
#include <AzQtComponents/Components/FancyDocking.h>
#include <AzQtComponents/Components/FancyDockingGhostWidget.h>
#include <AzQtComponents/Components/FancyDockingDropZoneWidget.h>
#include <AzQtComponents/Components/RepolishMinimizer.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/Titlebar.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/Utilities/QtWindowUtilities.h>
#include <AzQtComponents/Utilities/RandomNumberGenerator.h>
#include <AzQtComponents/Utilities/ScreenUtilities.h>

#include <QAbstractButton>
#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QEvent>
#include <QTimer>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QRandomGenerator>
#include <QScopedValueRollback>
#include <QScreen>
#include <QStyle>
#include <QStackedWidget>
#include <QStyleOptionToolButton>
#include <QVBoxLayout>
#include <QWindow>
#include <QtGui/private/qhighdpiscaling_p.h>


static void OptimizedSetParent(QWidget* widget, QWidget* parent)
{
    AzQtComponents::RepolishMinimizer minimizer; // Blocks useless polish requests caused by setParent()
    widget->setParent(parent);
}

namespace AzQtComponents
{

    // Constant for the threshold in pixels for snapping to edges while dragging for docking
    static const int g_snapThresholdInPixels = 15;

    static const QString MinimizeButtonObjectName =     QStringLiteral("minimizeButton");
    static const QString MaximizeButtonObjectName =     QStringLiteral("maximizeButton");
    static const QString CloseButtonObjectName =        QStringLiteral("closeButton");

    static Qt::Orientation orientation(Qt::DockWidgetArea area)
    {
        switch (area)
        {
        default:
        case Qt::BottomDockWidgetArea:
        case Qt::TopDockWidgetArea:
            return Qt::Vertical;
        case Qt::LeftDockWidgetArea:
        case Qt::RightDockWidgetArea:
            return Qt::Horizontal;
        }
    }

    /**
     * Stream operator for writing out the TabContainerType to a data stream
     */
    static QDataStream& operator<<(QDataStream& out, const FancyDocking::TabContainerType& myObj)
    {
        out << myObj.floatingDockName << myObj.tabNames << myObj.currentIndex;
        return out;
    }

    /**
     * Stream operator for reading in a TabContainerType from a data stream
     */
    static QDataStream& operator>>(QDataStream& in, FancyDocking::TabContainerType& myObj)
    {
        in >> myObj.floatingDockName;
        in >> myObj.tabNames;
        in >> myObj.currentIndex;
        return in;
    }

    namespace
    {
        static const char* g_AutoSavePropertyName = "AutoSaveLayout";

        static bool shouldSkipTitleBarOverdraw(QDockWidget* dockWidget)
        {
            StyledDockWidget* styledDockChild = qobject_cast<StyledDockWidget*>(dockWidget);
            if (styledDockChild != nullptr)
            {
                return styledDockChild->skipTitleBarOverdraw();
            }

            return false;
        }
    }

    /**
     * Create our fancy docking widget
     */
    FancyDocking::FancyDocking(DockMainWindow* mainWindow, const char* identifierPrefix)
        : QWidget(mainWindow, Qt::WindowFlags(Qt::ToolTip | Qt::BypassWindowManagerHint | Qt::FramelessWindowHint))
        , m_mainWindow(mainWindow)
        , m_emptyWidget(new QWidget(this))
        , m_dropZoneHoverFadeInTimer(new QTimer(this))
        , m_ghostWidget(new FancyDockingGhostWidget(mainWindow))
        , m_dropZoneWidgets()
        , m_floatingWindowIdentifierPrefix(QString("_%1_").arg(identifierPrefix))
        , m_tabContainerIdentifierPrefix(QString("_%1tabcontainer_").arg(identifierPrefix))
    {
        m_dropZoneState.setDropZoneColorOnHover(Style::dropZoneColorOnHover());

        // Register our TabContainerType stream operators so that they will be used
        // when reading/writing from/to data streams
        qRegisterMetaTypeStreamOperators<FancyDocking::TabContainerType>("FancyDocking::TabContainerType");
        mainWindow->installEventFilter(this);
        mainWindow->SetFancyDockingOwner(this);
        setAutoFillBackground(false);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_NoSystemBackground);

        // Make sure our placeholder empty widget is hidden by default
        m_emptyWidget->hide();

        // Update our docking overlay geometry, and listen for any changes to the
        // desktop screens being resized or added/removed so we can recalculate
        // our docking overlay
        updateDockingGeometry();

        for (auto screen : QApplication::screens())
        {
            QObject::connect(screen, &QScreen::geometryChanged, this, &FancyDocking::updateDockingGeometry);
        }

        QObject::connect(qApp, &QApplication::screenAdded, this, &FancyDocking::handleScreenAdded);
        QObject::connect(qApp, &QApplication::screenRemoved, this, &FancyDocking::handleScreenRemoved);

        // Timer for updating our hovered drop zone opacity
        QObject::connect(m_dropZoneHoverFadeInTimer, &QTimer::timeout, this, &FancyDocking::onDropZoneHoverFadeInUpdate);
        m_dropZoneHoverFadeInTimer->setInterval(FancyDockingDropZoneConstants::dropZoneHoverFadeUpdateIntervalMS);
        QIcon dragIcon = QIcon(QStringLiteral(":/Cursors/Grabbing.svg"));
        m_dragCursor = QCursor(dragIcon.pixmap(16), 5, 2);
    }

    FancyDocking::~FancyDocking()
    {
        for (auto i = m_dropZoneWidgets.begin(); i != m_dropZoneWidgets.end(); i++)
        {
            delete i.value();
        }
    }

    /**
     * Create a new QDockWidget whose main widget will be a DockMainWindow. It will be created floating
     * with the given geometry. The QDockWidget will be named with the given name
     */
    QMainWindow* FancyDocking::createFloatingMainWindow(const QString& name, const QRect& geometry, bool skipTitleBarOverdraw)
    {
        auto dockWidget = new StyledDockWidget(QString(), skipTitleBarOverdraw, m_mainWindow);
        dockWidget->setObjectName(name);
        if (!restoreDockWidget(dockWidget))
        {
            m_mainWindow->addDockWidget(Qt::LeftDockWidgetArea, dockWidget);
        }
        dockWidget->setFloating(true);

        // Make sure the floating dock container is deleted when closed so that
        // its children can be restored properly when re-opened (otherwise they
        // will try to show up on a floating dock widget that is invisible)
        dockWidget->setAttribute(Qt::WA_DeleteOnClose);

        // Stack this floating dock widget name on the top of our z-ordered list
        // since it was just created
        m_orderedFloatingDockWidgetNames.prepend(name);

        // Hide the title bar when the group is docked
        //commented out because our styled dockwidget takes care of this
        //connect(dockWidget, &QDockWidget::topLevelChanged, [dockWidget](bool fl)
        //    { if (!fl) dockWidget->setTitleBarWidget(new QWidget()); });
        DockMainWindow* mainWindow = new DockMainWindow(dockWidget);
        mainWindow->SetFancyDockingOwner(this);
        mainWindow->setWindowFlags(Qt::Widget);
        mainWindow->installEventFilter(this);

        dockWidget->setWidget(mainWindow);
        dockWidget->show();
        QRect adjustedGeometry = geometry;
        AzQtComponents::EnsureGeometryWithinScreenTop(adjustedGeometry);
        if (!adjustedGeometry.isNull())
        {
            dockWidget->setGeometry(adjustedGeometry);
        }

        return mainWindow;
    }

    /**
     * Create a new tab widget and a dock widget container to hold it
     */
    DockTabWidget* FancyDocking::createTabWidget(QMainWindow* mainWindow, QDockWidget* widgetToReplace, QString name)
    {
        // If a name wasn't provided, then generate a random one
        if (name.isEmpty())
        {
            name = getUniqueDockWidgetName(m_tabContainerIdentifierPrefix);
        }

        // Create a container dock widget for our tab widget
        StyledDockWidget* tabWidgetContainer = new StyledDockWidget(mainWindow);
        tabWidgetContainer->setObjectName(name);
        tabWidgetContainer->setFloating(false);

        // Set an empty QWidget as the custom title bar to hide it, since our tab widget will drive it's own custom tab bar
        // that will replace it (the empty QWidget is parented to the dock widget, so it will be cleaned up whenever the dock widget is deleted)
        tabWidgetContainer->setTitleBarWidget(new QWidget());

        // Create our new tab widget and listen for tab pressed, inserted, count changed, and undock events
        DockTabWidget* tabWidget = new DockTabWidget(m_mainWindow, mainWindow);
        QObject::connect(tabWidget, &DockTabWidget::tabIndexPressed, this, &FancyDocking::onTabIndexPressed);
        QObject::connect(tabWidget, &DockTabWidget::tabWidgetInserted, this, &FancyDocking::onTabWidgetInserted);
        QObject::connect(tabWidget, &DockTabWidget::tabCountChanged, this, &FancyDocking::onTabCountChanged);
        QObject::connect(tabWidget, &DockTabWidget::currentChanged, this, &FancyDocking::onCurrentTabIndexChanged);
        QObject::connect(tabWidget, &DockTabWidget::undockTab, this, &FancyDocking::onUndockTab);

        // Set our tab widget as the widget for our tab container docking widget
        tabWidgetContainer->setWidget(tabWidget);

        // There isn't a way to replace a dock widget in a layout, so we have to place our tab container dock widget
        // split next to our replaced widget, and then remove our replaced widget from the layout.  The replaced widget
        // will then be moved to our tab widget, so it effectively will remain in the same spot, but now it will be tabbed
        // instead of a standalone dock widget.
        if (widgetToReplace)
        {
            splitDockWidget(mainWindow, widgetToReplace, tabWidgetContainer, Qt::Horizontal);
            mainWindow->removeDockWidget(widgetToReplace);
            tabWidget->addTab(widgetToReplace);
        }

        return tabWidget;
    }

    /**
     * Return a unique object name with the specified prefix that doesn't collide with any QDockWidget children of our main window
     */
    QString FancyDocking::getUniqueDockWidgetName(const QString& prefix)
    {
        QString name;
        do
        {
            name = QString("%1%2").arg(prefix).arg(GetRandomGenerator()->generate(), 16);
        } while (m_mainWindow->findChild<QDockWidget*>(name));

        return name;
    }

    /**
     * Update the geometry of our docking overlay to be a union of all the screen
     * rects for each desktop monitor
     */
    void FancyDocking::updateDockingGeometry()
    {
        QRect totalScreenRect;
        int numScreens = QApplication::screens().count();

#ifdef AZ_PLATFORM_WINDOWS
        for (QWidget* w : m_perScreenFullScreenWidgets) {
            delete w;
        }
        m_perScreenFullScreenWidgets.clear();
#endif

        for (int i = 0; i < numScreens; ++i)
        {
#ifdef AZ_PLATFORM_WINDOWS
            QWidget* screenWidget = new QWidget(this);
            screenWidget->setGeometry(QApplication::screens().at(i)->geometry());
            m_perScreenFullScreenWidgets.push_back(screenWidget);
#else
            totalScreenRect = totalScreenRect.united(QApplication::screens().at(i)->geometry());
#endif
        }

#ifndef AZ_PLATFORM_WINDOWS
        setGeometry(totalScreenRect);
#endif

        // Update our list of screens whenever screens are added/removed so that we
        // don't have to query them every time
        m_desktopScreens = qApp->screens();
    }

    /**
     * Handle a screen being added to the current layout
     */
    void FancyDocking::handleScreenAdded(QScreen* screen)
    {
        QObject::connect(screen, &QScreen::geometryChanged, this, &FancyDocking::updateDockingGeometry);

        updateDockingGeometry();
    }
    
    /**
     * Handle a screen being removed from the current layout
     */
    void FancyDocking::handleScreenRemoved(QScreen* screen)
    {
        QObject::disconnect(screen, &QScreen::geometryChanged, this, &FancyDocking::updateDockingGeometry);

        updateDockingGeometry();
    }

    /**
     * Called on a timer interval to update the hovered drop zone opacity to make it
     * fade in with a set delay
     */
    void FancyDocking::onDropZoneHoverFadeInUpdate()
    {
        const qreal dropZoneHoverOpacity = FancyDockingDropZoneConstants::dropZoneHoverFadeIncrement + m_dropZoneState.dropZoneHoverOpacity();

        // Once we've reached the full drop zone opacity, cut it off in case we
        // went over and stop the timer
        if (dropZoneHoverOpacity >= FancyDockingDropZoneConstants::dropZoneOpacity)
        {
            m_dropZoneState.setDropZoneHoverOpacity(FancyDockingDropZoneConstants::dropZoneOpacity);
            m_dropZoneHoverFadeInTimer->stop();
        }
        else
        {
            m_dropZoneState.setDropZoneHoverOpacity(dropZoneHoverOpacity);
        }

        // Trigger a re-paint so the opacity will update
        RepaintFloatingIndicators();
    }

    /**
     * Return the number of visible dock widget children for the specified main window
     */
    int FancyDocking::NumVisibleDockWidgets(QMainWindow* mainWindow)
    {
        if (!mainWindow)
        {
            return -1;
        }

        // Count the number of visible dock widgets for our main window
        const QList<QDockWidget*> list = mainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly);
        ptrdiff_t count = std::count_if(list.cbegin(), list.cend(), [](QDockWidget* dockWidget) {
            return dockWidget->isVisible();
        });

        return aznumeric_cast<int>(count);
    }

    /**
     * Update the window title for the specified floating dock widget based on
     * one of its child dock widgets. This window title appears in the taskbar.
     * The update occurs on a SingleShot timer after any actions have completed.
     * Otherwise the dock widget may not be visible yet or the dock widget could
     * be a tab widget container that has not set its tab widget yet. Also, the
     * dock positioning may not yet be final
     */
    void FancyDocking::QueueUpdateFloatingWindowTitle(QMainWindow* mainWindow)
    {
        if (!mainWindow || mainWindow == m_mainWindow)
        {
            return;
        }

        StyledDockWidget* floatingDockWidget = qobject_cast<StyledDockWidget*>(mainWindow->parentWidget());
        if (!floatingDockWidget)
        {
            return;
        }

        QString floatingDockWidgetName = floatingDockWidget->objectName();
        QTimer::singleShot(0, m_mainWindow, [this, floatingDockWidgetName] {

            StyledDockWidget* floatingDockWidget = m_mainWindow->findChild<StyledDockWidget*>(floatingDockWidgetName, Qt::FindDirectChildrenOnly);
            if (!floatingDockWidget)
            {
                return;
            }

            QMainWindow* mainWindow = qobject_cast<QMainWindow*>(floatingDockWidget->widget());
            if (!mainWindow || mainWindow == m_mainWindow)
            {
                return;
            }

            const QDockWidget* topLeftWidget = nullptr;
            int topLeftWidgetManhattanDist = 0;
            for (QDockWidget* childDockWidget : mainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly))
            {
                if (!childDockWidget->isVisible())
                {
                    continue;
                }

                int childDockManhattanDist = childDockWidget->pos().manhattanLength();
                if (!topLeftWidget || childDockManhattanDist < topLeftWidgetManhattanDist)
                {
                    topLeftWidget = childDockWidget;
                    topLeftWidgetManhattanDist = childDockManhattanDist;
                }
            }

            QString title;
            if (topLeftWidget)
            {
                // Check if the child dock widget is a tab widget container
                DockTabWidget* tabWidget = qobject_cast<DockTabWidget*>(topLeftWidget->widget());
                if (tabWidget)
                {
                    int currentTabIndex = tabWidget->currentIndex();
                    if (currentTabIndex >= 0)
                    {
                        title = tabWidget->tabText(currentTabIndex);
                    }
                }
                else
                {
                    title = topLeftWidget->windowTitle();
                }
            }
            floatingDockWidget->setWindowTitle(title);
        });
    }
    
    /**
     * Adds close, maximize and minimize buttons from the DockTabWidget
     */
    void FancyDocking::AddTitleBarButtons(AzQtComponents::DockTabWidget* tabWidget, AzQtComponents::TitleBar* titleBar)
    {
        if (tabWidget->actions().count() > 0)
        {
            return;
        }

        // Show Action Toolbar
        tabWidget->setActionToolBarVisible(true);

        // Minimize Icon
        QAction* minimizeAction = new QAction(tr("Minimize"));
        minimizeAction->setObjectName(MinimizeButtonObjectName);

        connect(minimizeAction, &QAction::triggered, this, [titleBar]() {
            titleBar->handleMinimize();
        });

        tabWidget->addAction(minimizeAction);

        // Maximize Icon
        QAction* maximizeAction = new QAction(tr("Maximize"));
        maximizeAction->setObjectName(MaximizeButtonObjectName);

        connect(maximizeAction, &QAction::triggered, this, [titleBar]() {
            titleBar->handleMaximize();
        });

        tabWidget->addAction(maximizeAction);

        // Close Icon
        QAction* closeAction = new QAction(tr("Close"));
        closeAction->setObjectName(CloseButtonObjectName);

        connect(closeAction, &QAction::triggered, this, [titleBar]() {
            titleBar->handleClose();
        });

        tabWidget->addAction(closeAction);

        QObject::connect(tabWidget, &DockTabWidget::tabBarDoubleClicked, this, &FancyDocking::onTabBarDoubleClicked);

        // Toggle the TabBar and TitleBar to show the Window context menus
        auto dockTabBar = qobject_cast<AzQtComponents::DockTabBar*>(tabWidget->tabBar());
        if (dockTabBar)
        {
            dockTabBar->setIsShowingWindowControls(true);
        }

        titleBar->setIsShowingWindowControls(true);
    }
    
    /**
     * Remove close, maximize and minimize buttons from the DockTabWidget
     */
    void FancyDocking::RemoveTitleBarButtons(AzQtComponents::DockTabWidget* tabWidget, AzQtComponents::TitleBar* titleBar)
    {
        if (tabWidget->actions().count() == 0)
        {
            return;
        }

        // Hide Action Toolbar
        tabWidget->setActionToolBarVisible(false);

        for (QAction* action : tabWidget->actions())
        {
            tabWidget->removeAction(action);
        }

        QObject::disconnect(tabWidget, &DockTabWidget::tabBarDoubleClicked, this, &FancyDocking::onTabBarDoubleClicked);
        qobject_cast<DockTabBar*>(tabWidget->tabBar())->setIsShowingWindowControls(false);

        if (titleBar)
        {
            titleBar->setIsShowingWindowControls(false);
        }
    }
    
    /**
     * Update TitleBars for docked and floating widgets
     */
    void FancyDocking::UpdateTitleBars(QMainWindow* mainWindow)
    {
        if (!mainWindow)
        {
            return;
        }

        QList<QDockWidget*> childrenDockWidgets = mainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly);

        if (mainWindow == m_mainWindow)
        {
            // Main Window: remove all titlebar buttons from all widgets

            for (QDockWidget* dockWidget : childrenDockWidgets)
            {
                // Skip floating windows (as they also are direct children of the mainwindow)
                if (IsFloatingDockWidget(dockWidget))
                {
                    continue;
                }

                // TabWidgets
                for (auto tabWidget : dockWidget->findChildren<AzQtComponents::DockTabWidget*>(QString(), Qt::FindDirectChildrenOnly))
                {
                    RemoveTitleBarButtons(tabWidget);
                }

                // Regular DockWidgets
                if (auto innerTitleBar = qobject_cast<AzQtComponents::TitleBar*>(dockWidget->titleBarWidget()))
                {
                    innerTitleBar->setButtons({ });
                    innerTitleBar->setIsShowingWindowControls(false);
                }
            }
        }
        else
        {
            // Floating Window

            AzQtComponents::TitleBar* floatingWindowTitleBar = qobject_cast<StyledDockWidget*>(mainWindow->parentWidget())->customTitleBar();

            if (childrenDockWidgets.count() == 1)
            {
                // If there's just one child, TabWidgets and DockWidgets get the buttons and we hide the TitleBar

                // Hide TitleBar
                floatingWindowTitleBar->setDrawMode(AzQtComponents::TitleBar::TitleBarDrawMode::Hidden);

                // Show Buttons on TabWidgets
                auto tabWidgets = childrenDockWidgets[0]->findChildren<AzQtComponents::DockTabWidget*>(QString(), Qt::FindDirectChildrenOnly);
                for (auto tabWidget : tabWidgets)
                {
                    AddTitleBarButtons(tabWidget, floatingWindowTitleBar);
                }

                // Show Buttons on DockWidgets
                if (auto innerTitleBar = qobject_cast<AzQtComponents::TitleBar*>(childrenDockWidgets[0]->titleBarWidget()))
                {
                    innerTitleBar->setButtons({ DockBarButton::MinimizeButton, DockBarButton::MaximizeButton, DockBarButton::CloseButton });
                    innerTitleBar->setIsShowingWindowControls(true);
                }
            }
            else
            {
                // If multiple children are found, show the TitleBar and remove buttons from TabWidgets and DockWidgets

                // Show TitleBar with buttons
                floatingWindowTitleBar->setDrawMode(AzQtComponents::TitleBar::TitleBarDrawMode::Simple);
                floatingWindowTitleBar->setButtons({ DockBarButton::MinimizeButton, DockBarButton::MaximizeButton, DockBarButton::CloseButton });

                // Remove buttons from widgets
                for (QDockWidget* dockWidget : childrenDockWidgets)
                {
                    // TabWidgets
                    for (auto tabWidget : dockWidget->findChildren<AzQtComponents::DockTabWidget*>(QString(), Qt::FindDirectChildrenOnly))
                    {
                        RemoveTitleBarButtons(tabWidget, floatingWindowTitleBar);
                    }
                    
                    // DockWidget
                    if (auto innerTitleBar = qobject_cast<AzQtComponents::TitleBar*>(dockWidget->titleBarWidget()))
                    {
                        innerTitleBar->setButtons({ });
                        innerTitleBar->setIsShowingWindowControls(false);
                    }
                }
            }
        }
    }
    
    /**
     * Returns true if the DockWidget is a FancyDocking floating window
     */
    bool FancyDocking::IsFloatingDockWidget(QDockWidget* dockWidget)
    {
        QString dockName = dockWidget->objectName();
        return dockName.startsWith(m_floatingWindowIdentifierPrefix);
    }

    /**
     * Adjust mapFromGlobal to account for DPI scaling on multiple screens
     */
    QPoint FancyDocking::multiscreenMapFromGlobal(const QPoint& point) const
    {
#if 0 //def AZ_PLATFORM_WINDOWS
        int index = 0;
        for (auto screen : QApplication::screens()) {
            if (screen->geometry().contains(point)) {
                qreal scaleFactor = QHighDpiScaling::factor(screen);
                return (
                    (m_perScreenFullScreenWidgets[index]->mapFromGlobal(point) * scaleFactor) +
                    (m_perScreenFullScreenWidgets[index]->mapToGlobal({0, 0})) / scaleFactor);
            }
            ++index;
        }

        // If the point isn't contained in any screen, return the regular mapFromGlobal() result for now
        // TODO - may need to do some shenanigan like the above based to the closest screen?
        return mapFromGlobal(point);
#else
        return mapFromGlobal(point);
#endif
    }

    bool FancyDocking::WidgetContainsPoint(QWidget* widget, const QPoint& pos) const
    {
        if (!widget)
        {
            return false;
        }

        QPoint globalPos = multiscreenMapFromGlobal(pos);

        // Find out the true global position of the widgets top left coordinate
        QPoint widgetTopLeft = multiscreenMapFromGlobal(widget->mapToGlobal(QPoint(0, 0)));

        // Construct the global rect given our widgets top left coordinate plus size
        QRect widgetGlobalRect(widgetTopLeft, QSize(widget->width(), widget->height()));

        return widgetGlobalRect.contains(globalPos);
    }

    /**
     * Destroy a floating main window if it no longer contains any QDockWidgets
     */
    void FancyDocking::destroyIfUseless(QMainWindow* mainWindow)
    {
        // Ignore if this was triggered on our main window, or if this is triggered
        // during a tabify action, during which the dock widgets may be hidden
        // so it ends up deleting the floating main window
        if (!mainWindow || mainWindow == m_mainWindow || m_state.updateInProgress)
        {
            return;
        }

        // Remove the container main window if there are no more visible QDockWidgets
        int count = NumVisibleDockWidgets(mainWindow);
        if (count > 0)
        {
            return;
        }

        // Avoid a recursion
        mainWindow->removeEventFilter(this);

        // Verify the parent is a styled dock widget
        auto floatingDockWidget = qobject_cast<AzQtComponents::StyledDockWidget*>(mainWindow->parentWidget());

        if (!floatingDockWidget)
        {
            AZ_Warning("FancyDocking", false, "Error - Floating dock widget parent isn't a StyledDockWidget.");
            return;
        }

        // Save the state of this floating dock widget that's about to be destroyed
        // so that we can re-create it if necessary when restoring any panes whose
        // last location was in this floating dock widget
        QString floatingDockWidgetName = floatingDockWidget->objectName();
        if (!floatingDockWidgetName.isEmpty())
        {
            m_restoreFloatings[floatingDockWidgetName] = qMakePair(mainWindow->saveState(), floatingDockWidget->geometry());
        }

        // Any dock widgets left in our floating main window were hidden, so
        // reparent them to the editor main window and make sure they remain
        // hidden.  This is so they will be restored properly the next time
        // someone tries to open them, because otherwise, it would try to
        // open them on the floating main window that no longer exists.
        for (QDockWidget* dockWidget : mainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly))
        {
            OptimizedSetParent(dockWidget, m_mainWindow);
            dockWidget->setVisible(false);
        }

        // Remove this floating dock widget from our z-ordered list of dock widget names
        m_orderedFloatingDockWidgetNames.removeAll(floatingDockWidgetName);

        // If a top level window has the dock widget we're about to destroy as transient parent,
        // update its transient parent to that of the dock widget, so that z-ordering is maintained.
        if (const QWindow* dockWidgetWindow = floatingDockWidget->windowHandle())
        {
            for (QWidget* widget : QApplication::topLevelWidgets())
            {
                if (QWindow* window = widget->windowHandle())
                {
                    if (window->transientParent() == dockWidgetWindow)
                    {
                        window->setTransientParent(dockWidgetWindow->transientParent());
                    }
                }
            }
        }

        // Lastly, delete our empty floating dock widget container, which will
        // also delete the floating main window since it is a child.
        floatingDockWidget->deleteLater();
    }

    /**
     * Return an absolute drop zone (if applicable) for the given drop target
     */
    QRect FancyDocking::getAbsoluteDropZone(QWidget* dock, Qt::DockWidgetArea& area, const QPoint& globalPos)
    {
        QRect absoluteDropZoneRect;
        if (!dock || ForceTabbedDocksEnabled())
        {
            return absoluteDropZoneRect;
        }

        // Check if we are trying to drop onto a main window, and if not, get the
        // main window from the drop target parent
        QMainWindow* mainWindow = qobject_cast<QMainWindow*>(dock);
        bool dropTargetIsMainWindow = true;
        if (!mainWindow)
        {
            dropTargetIsMainWindow = false;
            mainWindow = qobject_cast<QMainWindow*>(dock->parentWidget());
        }

        // If we still couldn't find a valid main window, then bail out
        if (!mainWindow)
        {
            return absoluteDropZoneRect;
        }

        // Don't allow the dragged dock widget to be docked as absolute
        // if it's already in the target main window and there is only
        // one other widget alongside it
        if (mainWindow != m_mainWindow)
        {
            const auto childDockWidgets = mainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly);
            if (childDockWidgets.size() <= 2 && childDockWidgets.contains(m_state.dock))
            {
                return absoluteDropZoneRect;
            }
        }

        // Setup the possible absolute drop zones for the given main window
        QRect mainWindowRect(mainWindow->rect());
        QPoint mainWindowTopLeft = multiscreenMapFromGlobal(mainWindow->mapToGlobal(mainWindowRect.topLeft()));
        QPoint mainWindowTopRight = multiscreenMapFromGlobal(mainWindow->mapToGlobal(mainWindowRect.topRight()));
        QPoint mainWindowBottomLeft = multiscreenMapFromGlobal(mainWindow->mapToGlobal(mainWindowRect.bottomLeft()));
        QSize absoluteLeftRightSize(FancyDockingDropZoneConstants::absoluteDropZoneSizeInPixels, mainWindowRect.height());
        QRect absoluteLeftDropZone(mainWindowTopLeft, absoluteLeftRightSize);
        QRect absoluteRightDropZone(mainWindowTopRight - QPoint(FancyDockingDropZoneConstants::absoluteDropZoneSizeInPixels, 0), absoluteLeftRightSize);
        QSize absoluteTopBottomSize(mainWindowRect.width(), FancyDockingDropZoneConstants::absoluteDropZoneSizeInPixels);
        QRect absoluteTopDropZone(mainWindowTopLeft, absoluteTopBottomSize);
        QRect absoluteBottomDropZone(mainWindowBottomLeft - QPoint(0, FancyDockingDropZoneConstants::absoluteDropZoneSizeInPixels), absoluteTopBottomSize);

        // If the drop target is a main window, then we will only show the absolute
        // drop zone if the cursor is in that zone already
        if (dropTargetIsMainWindow)
        {
            QPoint localPos = multiscreenMapFromGlobal(globalPos);

            if (absoluteLeftDropZone.contains(localPos))
            {
                absoluteDropZoneRect = absoluteLeftDropZone;
                area = Qt::LeftDockWidgetArea;
            }
            else if (absoluteRightDropZone.contains(localPos))
            {
                absoluteDropZoneRect = absoluteRightDropZone;
                area = Qt::RightDockWidgetArea;
            }
            else if (absoluteTopDropZone.contains(localPos))
            {
                absoluteDropZoneRect = absoluteTopDropZone;
                area = Qt::TopDockWidgetArea;
            }
            else if (absoluteBottomDropZone.contains(localPos))
            {
                absoluteDropZoneRect = absoluteBottomDropZone;
                area = Qt::BottomDockWidgetArea;
            }
        }
        // Otherwise if the drop target is just a normal dock widget, then we will
        // show the absolute drop zone once a normal drop zone sharing that edge
        // is activated
        else
        {
            const QRect& dockRect = dock->rect();
            QPoint dockTopLeft = multiscreenMapFromGlobal(dock->mapToGlobal(dockRect.topLeft()));
            QPoint dockBottomRight = multiscreenMapFromGlobal(dock->mapToGlobal(dockRect.bottomRight()));
            area = m_dropZoneState.dropArea();

            // If the hovered over drop zone shares a side with an absolute edge, then we need to setup
            // an absolute drop zone for that area (if absolute drop zones are allowed for this target)
            switch (m_dropZoneState.dropArea())
            {
            case Qt::LeftDockWidgetArea:
                if (dockTopLeft.x() == mainWindowTopLeft.x())
                {
                    absoluteDropZoneRect = absoluteLeftDropZone;
                }
                break;
            case Qt::RightDockWidgetArea:
                if (dockBottomRight.x() == mainWindowTopRight.x())
                {
                    absoluteDropZoneRect = absoluteRightDropZone;
                }
                break;
            case Qt::TopDockWidgetArea:
                if (dockTopLeft.y() == mainWindowTopLeft.y())
                {
                    absoluteDropZoneRect = absoluteTopDropZone;
                }
                break;
            case Qt::BottomDockWidgetArea:
                if (dockBottomRight.y() == mainWindowBottomLeft.y())
                {
                    absoluteDropZoneRect = absoluteBottomDropZone;
                }
                break;
            }
        }

        return absoluteDropZoneRect;
    }

    /**
     * Set m_dropZoneState.dropOnto and the m_dropZoneState.dropZones as to drop within the specified dock.
     */
    void FancyDocking::setupDropZones(QWidget* dock, const QPoint& globalPos)
    {
        // If there is no dock widget, then reset our drop zones and return
        if (!dock)
        {
            m_dropZoneState.setDropOnto(dock);
            m_dropZoneState.setDropZones({});
            m_dropZoneState.setDockDropZoneRect(QRect());
            m_dropZoneState.setInnerDropZoneRect(QRect());
            m_dropZoneState.setAbsoluteDropZoneArea(Qt::NoDockWidgetArea);
            m_dropZoneState.setAbsoluteDropZoneRect(QRect());
            return;
        }

        // If the drop widget is a QMainWindow, then we won't show the normal drop zones
        QMainWindow* mainWindow = qobject_cast<QMainWindow*>(dock);
        bool normalDropZonesAllowed = !mainWindow;

        // Figure out if we need to recalculate the drop zones
        QRect dockRect = dock->rect();
        if (m_dropZoneState.dropOnto() == dock)
        {
            if (mainWindow)
            {
                // If the drop target is a main window, this means the mouse is
                // hovered over a dead zone margin, the central widget (viewport),
                // or the widget that is being dragged, so we will need to setup
                // an absolute drop zone based on the mouse position
                if (m_dropZoneState.onAbsoluteDropZone())
                {
                    // If we're already hovered on the applicable absolute
                    // drop zone, then we don't need to re-calculate
                    return;
                }
                else
                {
                    Qt::DockWidgetArea area = Qt::NoDockWidgetArea;
                    m_dropZoneState.setAbsoluteDropZoneRect(getAbsoluteDropZone(dock, area, globalPos));
                    m_dropZoneState.setAbsoluteDropZoneArea(area);
                }
            }
            else if (m_dropZoneState.dropArea() == Qt::NoDockWidgetArea || m_dropZoneState.dropArea() == Qt::AllDockWidgetAreas)
            {
                // If we're hovered over the dead zone or the center tab, then reset the absolute drop
                // zone if there is one so we can recalculate the drop zones
                if (m_dropZoneState.absoluteDropZoneRect().isValid())
                {
                    m_dropZoneState.setAbsoluteDropZoneArea(Qt::NoDockWidgetArea);
                    m_dropZoneState.setAbsoluteDropZoneRect(QRect());
                }
                // Otherwise the drop zones don't need to be updated, so return
                else
                {
                    return;
                }
            }
            else
            {
                // If we're still hovered over the same area, no need to re-calculate the absolute drop zones
                if (m_dropZoneState.absoluteDropZoneArea() == m_dropZoneState.dropArea())
                {
                    return;
                }


                // Try to setup an absolute drop zone based on the dock widget
                Qt::DockWidgetArea area = Qt::NoDockWidgetArea;
                QRect absoluteDropZoneRect = getAbsoluteDropZone(dock, area);

                // If we setup an absolute drop zone, then cache it
                if (absoluteDropZoneRect.isValid())
                {
                    m_dropZoneState.setAbsoluteDropZoneRect(absoluteDropZoneRect);
                    m_dropZoneState.setAbsoluteDropZoneArea(area);
                }
                // If the current area doesn't need an absolute drop zone, and we didn't have an absolute drop zone previously,
                // then we don't need to make any changes so return
                else if (!m_dropZoneState.absoluteDropZoneRect().isValid())
                {
                    return;
                }
                // Otherwise clear out our cached absolute drop zone so we can reset everything
                else
                {
                    m_dropZoneState.setAbsoluteDropZoneArea(Qt::NoDockWidgetArea);
                    m_dropZoneState.setAbsoluteDropZoneRect(QRect());
                }
            }
        }
        // We switched drop widgets; clear out the absolute drop zone data
        else
        {
            m_dropZoneState.setAbsoluteDropZoneArea(Qt::NoDockWidgetArea);
            m_dropZoneState.setAbsoluteDropZoneRect(QRect());
        }

        // We need to recalculate the drop zones, so clear them and proceed
        m_dropZoneState.setDropOnto(dock);
        m_dropZoneState.setDropZones({});
        m_dropZoneState.setInnerDropZoneRect(QRect());
        StartDropZone(m_dropZoneState.dropOnto(), globalPos);

        // Don't setup the normal drop zones if our drop target is a QMainWindow
        if (!normalDropZonesAllowed)
        {
            raiseDockWidgets();
            return;
        }

        // If there is a valid absolute drop zone, adjust our outer dock widget rectangle accordingly to make room for it
        switch (m_dropZoneState.absoluteDropZoneArea())
        {
        case Qt::LeftDockWidgetArea:
            dockRect.setX(dockRect.x() + FancyDockingDropZoneConstants::absoluteDropZoneSizeInPixels);
            break;
        case Qt::RightDockWidgetArea:
            dockRect.setWidth(dockRect.width() - FancyDockingDropZoneConstants::absoluteDropZoneSizeInPixels);
            break;
        case Qt::TopDockWidgetArea:
            dockRect.setY(dockRect.y() + FancyDockingDropZoneConstants::absoluteDropZoneSizeInPixels);
            break;
        case Qt::BottomDockWidgetArea:
            dockRect.setHeight(dockRect.height() - FancyDockingDropZoneConstants::absoluteDropZoneSizeInPixels);
            break;
        }

        // Store our potentially adjusted outer dock widget rectangle and retrieve its corner points for later calculations
        m_dropZoneState.setDockDropZoneRect(dockRect);
        const QPoint topLeft = multiscreenMapFromGlobal(dock->mapToGlobal(dockRect.topLeft()));
        const QPoint topRight = multiscreenMapFromGlobal(dock->mapToGlobal(dockRect.topRight()));
        const QPoint bottomLeft = multiscreenMapFromGlobal(dock->mapToGlobal(dockRect.bottomLeft()));
        const QPoint bottomRight = multiscreenMapFromGlobal(dock->mapToGlobal(dockRect.bottomRight()));

        /*
            The normal drop zones for left/right/top/bottom of a dock widget are trapezoids with the longer
            side on the edges of the widget, and the shorter side towards the middle of the widget. Here
            is a rough depiction:
             _______________________
            |\                     /|
            | \                   / |
            |  \_________________/  |
            |   |               |   |
            |   |               |   |
            |   |               |   |
            |   |_______________|   |
            |  /                 \  |
            | /                   \ |
            |/_____________________\|
            The drop zones are constructed using polygons with the appropriate points from the dock widget
            and the calculated inner points.
        */
        int dockWidth = dockRect.width();
        int dockHeight = dockRect.height();
        int topLeftX = topLeft.x();
        int topLeftY = topLeft.y();
        int topRightX = topRight.x();
        int bottomLeftY = bottomLeft.y();
    
        // Set the drop zone width/height to the default, but if the dock widget
        // width and/or height is below the threshold, then switch to scaling them
        // down accordingly
        int dropZoneWidth = FancyDockingDropZoneConstants::dropZoneSizeInPixels;
        if (dockWidth < FancyDockingDropZoneConstants::minDockSizeBeforeDropZoneScalingInPixels)
        {
            dropZoneWidth = aznumeric_cast<int>(dockWidth * FancyDockingDropZoneConstants::dropZoneScaleFactor);
        }
        int dropZoneHeight = FancyDockingDropZoneConstants::dropZoneSizeInPixels;
        if (dockHeight < FancyDockingDropZoneConstants::minDockSizeBeforeDropZoneScalingInPixels)
        {
            dropZoneHeight = aznumeric_cast<int>(dockHeight * FancyDockingDropZoneConstants::dropZoneScaleFactor);
        }

        // Calculate the inner corners to be used when constructing the drop zone polygons
        QPoint innerTopLeft(topLeftX + dropZoneWidth, topLeftY + dropZoneHeight);
        QPoint innerTopRight(topRightX - dropZoneWidth, topLeftY + dropZoneHeight);
        QPoint innerBottomLeft(topLeftX + dropZoneWidth, bottomLeftY - dropZoneHeight);
        QPoint innerBottomRight(topRightX - dropZoneWidth, bottomLeftY - dropZoneHeight);
        m_dropZoneState.setInnerDropZoneRect(QRect(innerTopLeft, innerBottomRight));

        auto dropZones = m_dropZoneState.dropZones();

        // Only setup the left/right/top/bottom drop zones if our main window doesn't
        // have the force tabbed docks only flag set.
        if (!ForceTabbedDocksEnabled())
        {
            // Setup the left/right/top/bottom drop zones using our calculated points
            QPolygon leftDropZone, rightDropZone, topDropZone, bottomDropZone;
            leftDropZone << topLeft << innerTopLeft << innerBottomLeft << bottomLeft;
            rightDropZone << topRight << bottomRight << innerBottomRight << innerTopRight;
            topDropZone << topLeft << topRight << innerTopRight << innerTopLeft;
            bottomDropZone << bottomLeft << innerBottomLeft << innerBottomRight << bottomRight;

            dropZones[Qt::LeftDockWidgetArea] = leftDropZone;
            dropZones[Qt::RightDockWidgetArea] = rightDropZone;
            dropZones[Qt::TopDockWidgetArea] = topDropZone;
            dropZones[Qt::BottomDockWidgetArea] = bottomDropZone;
        }

        // Add the center drop zone for docking as a tab. The drop zone will be
        // stored as a polygon, although it will actually be drawn/evaluated
        // as a circle. The center drop zone size will be whichever is smaller
        // between the inner drop zone width vs height, and scaled accordingly
        int innerDropZoneWidth = m_dropZoneState.innerDropZoneRect().width();
        int innerDropZoneHeight = m_dropZoneState.innerDropZoneRect().height();
        int centerDropZoneDiameter = (innerDropZoneWidth < innerDropZoneHeight) ? innerDropZoneWidth : innerDropZoneHeight;
        centerDropZoneDiameter = aznumeric_cast<int>(centerDropZoneDiameter * FancyDockingDropZoneConstants::centerTabDropZoneScale);

        // Setup our center tab drop zone
        const QSize centerDropZoneSize(centerDropZoneDiameter, centerDropZoneDiameter);
        const QRect centerDropZoneRect(m_dropZoneState.innerDropZoneRect().center() - QPoint(centerDropZoneDiameter, centerDropZoneDiameter) / 2, centerDropZoneSize);
        dropZones[Qt::AllDockWidgetAreas] = QPolygon(centerDropZoneRect, true);     // AllDockWidgetAreas means we want tab

        m_dropZoneState.setDropZones(dropZones);

        // Make sure the drop zones don't overlap with floating dock windows in the foreground
        raiseDockWidgets();
    }

    /**
     * Raise the appropriate dock widgets given the current widget to be dropped on
     * so that the drop zones don't overlap with floating dock windows in the foreground
     */
    void FancyDocking::raiseDockWidgets()
    {
        QWidget* dropOnto = m_dropZoneState.dropOnto();
        if (!dropOnto)
        {
            return;
        }

        // If our drop target isn't a main window, then retrieve the main window
        // from the dock widget parent
        QMainWindow* mainWindow = qobject_cast<QMainWindow*>(dropOnto);
        if (!mainWindow)
        {
            mainWindow = qobject_cast<QMainWindow*>(dropOnto->parentWidget());
        }

        if (mainWindow && mainWindow != m_mainWindow)
        {
            // If our dock widget is part of a floating main window, then we need
            // to retrieve its container dock widget to raise that to the
            // foreground and then raise our docking overlay on top
            QDockWidget* containerDockWidget = qobject_cast<QDockWidget*>(mainWindow->parentWidget());
            if (containerDockWidget)
            {
                containerDockWidget->raise();
            }
        }

        if (m_activeDropZoneWidgets.size())
        {
            // the floating dropzone indicators clip against everything above them
            // so they should always be on top of everything else
            for (FancyDockingDropZoneWidget* dropZoneWidget : m_activeDropZoneWidgets)
            {
                dropZoneWidget->raise();
            }
        }

        // floating pixmap is always on top; it'll clip what it's supposed to
        m_ghostWidget->raise();
    }

    /*!
      Return on which dockArea should we drop something depending on the global position of the cursor
      */
    Qt::DockWidgetArea FancyDocking::dockAreaForPos(const QPoint& globalPos)
    {
        m_dropZoneState.setOnAbsoluteDropZone(false);
        if (!m_dropZoneState.dropOnto())
        {
            return Qt::NoDockWidgetArea;
        }
        const QPoint& pos = multiscreenMapFromGlobal(globalPos);

        // First, check if we are hovered over an absolute drop zone
        if (m_dropZoneState.absoluteDropZoneRect().isValid() && m_dropZoneState.absoluteDropZoneRect().contains(pos))
        {
            m_dropZoneState.setOnAbsoluteDropZone(true);
            return m_dropZoneState.absoluteDropZoneArea();
        }

        // Then, check all of the default drop zones
        auto dropZones = m_dropZoneState.dropZones();
        for (auto it = dropZones.cbegin(); it != dropZones.cend(); ++it)
        {
            const Qt::DockWidgetArea area = it.key();
            const QPolygon& dropZoneShape = it.value();

            // For the center tab drop zone, we need to translate the shape into a circle before we
            // check if the mouse position is inside the shape.
            if (area == Qt::AllDockWidgetAreas)
            {
                QRegion circleRegion(dropZoneShape.boundingRect(), QRegion::Ellipse);
                if (circleRegion.contains(pos))
                {
                    return area;
                }
            }
            // For the left/right/top/bottom drop zones we can use the default polygon check if the mouse
            // position is inside the shape
            else
            {
                if (dropZoneShape.containsPoint(pos, Qt::OddEvenFill))
                {
                    return area;
                }
            }
        }
 
        return Qt::NoDockWidgetArea;
    }

    /**
     * For a given widget, determine if it is a valid drop target and return the
     * valid drop target if applicable. If the drop target is excluded (e.g. we
     * are dragging this widget), then its parent main window will be returned.
     */
    QWidget* FancyDocking::dropTargetForWidget(QWidget* widget, const QPoint& globalPos, QWidget* exclude) const
    {
        auto isExcluded = [&](const QWidget* x)
        {
            for (auto i = x; i; i = i->parentWidget())
            {
                if (i == exclude)
                {
                    return true;
                }
            }
            return false;
        };

        if (!widget || widget->isHidden())
        {
            return nullptr;
        }

        if (isExcluded(widget))
        {
            // If the mouse is over our excluded widget, then return its parent
            // instead so we can still evaluate for absolute drop zones
            if (WidgetContainsPoint(widget, globalPos))
            {
                return qobject_cast<QMainWindow*>(widget->parentWidget());
            }
            else
            {
                return nullptr;
            }
        }

        if (WidgetContainsPoint(widget, globalPos))
        {
            return widget;
        }

        return nullptr;
    }

    /**
     * Given a position in global coordinates, returns a QDockWidget, or a QMainWindow onto which
     * one can drop a widget.  This exclude the 'exclude' widget and all it's children.
     */
    QWidget* FancyDocking::dropWidgetUnderMouse(const QPoint& globalPos, QWidget* exclude) const
    {
        // After this logic block, this will hold a valid QMainWindow reference if
        // our current drop target is on a floating main window
        QMainWindow* dropOntoFloatingMainWindow = nullptr;
        QWidget* dropOnto = m_dropZoneState.dropOnto();
        if (qobject_cast<QDockWidget*>(dropOnto))
        {
            // If our drop target is a dock widget, then check if its parent is
            // a floating main window
            QMainWindow* mainWindow = qobject_cast<QMainWindow*>(dropOnto->parentWidget());
            if (mainWindow != m_mainWindow)
            {
                // If we're still hovered over the same dock widget, this shortcuts
                // all the logic below
                if (WidgetContainsPoint(dropOnto, globalPos))
                {
                    return dropOnto;
                }
                // Otherwise, our mouse still be hovered over the same floating
                // window, so we need to give it precedence over other floating
                // main windows and the main editor window
                else
                {
                    dropOntoFloatingMainWindow = mainWindow;
                }
            }
        }
        else if (dropOnto && dropOnto != m_mainWindow)
        {
            // If we have a valid drop target and it wasn't a dock widget, then
            // it's a QMainWindow so we need to flag it if it's floating
            dropOntoFloatingMainWindow = qobject_cast<QMainWindow*>(dropOnto);
        }

        // Create a list of our floating drop targets separate from the dock widgets
        // on our main editor window so we can give precedence to the floating targets
        // We iterate through our floating drop targets by our z-ordered list of
        // floating dock widgets that we maintain ourselves since we can't retrieve
        // a z-ordered list from Qt, and we need to guarantee that dock widgets
        // in the front have precedence over widgets that are lower
        QList<QWidget*> floatingDropTargets;
        for (QString name : m_orderedFloatingDockWidgetNames)
        {
            QDockWidget* dockWidget = m_mainWindow->findChild<QDockWidget*>(name, Qt::FindDirectChildrenOnly);
            if (!dockWidget)
            {
                continue;
            }

            // Make sure this is a floating dock widget container
            // We need to add its dock widget children and floating main window as
            // drop targets
            if (!dockWidget->isFloating())
            {
                continue;
            }

            // Ignore this floating container if it is hidden, which means it
            // is a single pane floating window that is the one being dragged
            // so it is currently hidden
            if (dockWidget->isHidden())
            {
                continue;
            }

            // Ignore this floating container it the window is minimized
            if (dockWidget->isMinimized())
            {
                continue;
            }

            QMainWindow* mainWindow = qobject_cast<QMainWindow*>(dockWidget->widget());
            if (!mainWindow)
            {
                continue;
            }

            // If our current drop target lives in this floating main window,
            // then we need to add it to the front of the list so that it will
            // get precedence over other floating windows, but we need to do this
            // first so that the dock widgets of this main window will be prepended
            // in front of it
            if (mainWindow == dropOntoFloatingMainWindow)
            {
                floatingDropTargets.prepend(mainWindow);
            }

            // Add all of the child dock widgets in this floating main window
            // to our list of floating drop targets
            bool shouldAddFloatingMainWindow = true;
            for (QDockWidget* floatingDockWidget : mainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly))
            {
                // Don't allow dock widgets that have no allowed areas to be
                // drop targets, and also prevent this floating main window
                // from being added as a drop target as well if it contains
                // a dock widget that has docking disabled
                if (floatingDockWidget->allowedAreas() == Qt::NoDockWidgetArea)
                {
                    shouldAddFloatingMainWindow = false;
                    continue;
                }

                // If our current drop target lives in this floating main window,
                // then put these dock widgets on the front of our list so they
                // get precedence over other floating drop targets
                if (mainWindow == dropOntoFloatingMainWindow)
                {
                    floatingDropTargets.prepend(floatingDockWidget);
                }
                // Otherwise just add them to the list of other floating drop targets
                else
                {
                    floatingDropTargets.append(floatingDockWidget);
                }
            }

            // If our current drop target does not live in this floating main
            // window, then store this floating main window in our list of
            // floating drop targets after its dock widgets so that they will
            // be found first
            if (shouldAddFloatingMainWindow && mainWindow != dropOntoFloatingMainWindow)
            {
                floatingDropTargets.append(mainWindow);
            }
        }

        // Then, find the normal dock widgets on the main editor window and add
        // them to the end of list so the floating widgets have priority
        QList<QDockWidget*> mainWindowDockWidgets;
        if (!m_mainWindow->window()->isMinimized())
        {
            for (QDockWidget* dockWidget : m_mainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly))
            {
                if (!dockWidget->isFloating())
                {
                    mainWindowDockWidgets.append(dockWidget);
                }
            }
        }

        // Next, check all of the floating drop targets. This includes the floating
        // dock widgets, and the floating main windows themselves so we catch the
        // absolute drop zones when hovered over the dead zone margins or the excluded
        // target (widget being dragged).
        for (QWidget* widget : floatingDropTargets)
        {
            QWidget* dropTarget = dropTargetForWidget(widget, globalPos, exclude);
            if (dropTarget)
            {
                return dropTarget;
            }
        }

        // Then, check all the dock widgets on the main window
        for (QDockWidget* dockWidget : mainWindowDockWidgets)
        {
            QWidget* dropTarget = dropTargetForWidget(dockWidget, globalPos, exclude);
            if (dropTarget)
            {
                return dropTarget;
            }
        }

        // Fallback to check if the mouse is inside our main window, which will cover
        // both the central widget (viewport) and the dead zone margins between
        // dock widgets on the main window
        if (m_mainWindow->rect().contains(m_mainWindow->mapFromGlobal(globalPos)) && !m_mainWindow->window()->isMinimized())
        {
            return m_mainWindow;
        }

        return nullptr;
    }

    /**
     * Handle a mouse move event.
     */
    bool FancyDocking::dockMouseMoveEvent(QDockWidget* dock, QMouseEvent* event)
    {
        if (!m_state.dock)
        {
            return false;
        }

        // If we are dragging a floating dock widget, then we need to use the
        // actual dock widget child as our reference
        if (m_state.floatingDockContainer && m_state.floatingDockContainer == dock)
        {
            dock = m_state.dock;
        }

        if (m_state.dock != dock)
        {
            return false;
        }

        // use QCursor::pos(); in scenarios with multiple screens and different scale factors,
        // it's much more reliable about actually reporting a global position than
        // using event->globalPos();
        QPoint globalPos = QCursor::pos();

        if (!m_dropZoneState.dragging())
        {
            // Check if we should start dragging if the user has pressed and dragged
            // the mouse beyond the drag distance threshold, taking into account the
            // title bar height if we are dragging by the floating title bar
            QPoint dragDifference = globalPos - dock->mapToGlobal(m_state.pressPos);
            if (m_state.floatingDockContainer)
            {
                dragDifference.ry() += dock->titleBarWidget()->height();
            }
            bool shouldStartDrag = dragDifference.manhattanLength() > QApplication::startDragDistance();

            // Only initiate the tab re-ordering logic for tab widgets that have
            // multiple tabs
            if (m_state.tabWidget && m_state.tabWidget->count() > 1)
            {
                // If we are dragging a tab, we shouldn't rip the tab out until the
                // mouse leaves the tab header area
                QTabBar* tabBar = m_state.tabWidget->tabBar();
                shouldStartDrag = !tabBar->rect().contains(tabBar->mapFromGlobal(globalPos));

                // If the tab has been ripped out, we need to reset the tab widget's
                // internal drag state
                if (shouldStartDrag)
                {
                    m_state.tabWidget->finishDrag();
                }
                // Otherwise, the mouse is still being dragged inside the tab header
                // area, so pass the mouse event along to the tab widget so it can
                // use it for internally dragging the tabs to re-order them, and
                // bail out since the tab widget will handle this mouse event
                else
                {
                    // Construct a new QMouseEvent with a local mouse position that is correct for
                    // the tab widget. We can't just pass the event being filtered because the mouse
                    // positions are relative to the widget being watched.
                    const auto tabPos = m_state.tabWidget->mapFromGlobal(event->globalPos());
                    QMouseEvent tabEvent(event->type(), tabPos, event->button(), event->buttons(), event->modifiers());
                    m_state.tabWidget->mouseMoveEvent(&tabEvent);
                    return true;
                }
            }

            // If we shouldn't start the drag, then bail out, otherwise we will
            // rip out the dock widget and start the dragging process
            if (!shouldStartDrag)
            {
                return false;
            }

            m_ghostWidget->show();

            // We need to explicitly grab the mouse/keyboard on our main window when
            // we start dragging a dock widget so that only our custom docking logic
            // will be executed, instead of qt's default docking.  This also allows
            // us to hide the dock widget if it's floating and still receive the events
            // since otherwise they would be lost if the widget was hidden.
            m_mainWindow->grabMouse();
            m_mainWindow->grabKeyboard();

            // If we're dragging a dock widget that is the only widget in a floating
            // window, let's hide the floating window so it doesn't get in the way.
            // If the dock widget is a tab container, then we will only hide it if
            // it only has one tab.
            QDockWidget* singleFloatingDockWidget = nullptr;
            QMainWindow* mainWindow = qobject_cast<QMainWindow*>(dock->parentWidget());

            if (mainWindow && mainWindow != m_mainWindow)
            {
                QDockWidget* containerDockWidget = qobject_cast<QDockWidget*>(mainWindow->parentWidget());
                if (containerDockWidget && containerDockWidget->isFloating())
                {
                    int numVisibleDockWidgets = 0;
                    for (QDockWidget* dockWidget : mainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly))
                    {
                        if (dockWidget->isVisible())
                        {
                            // If we're only dragging one tab out of a tabWidget, count all tabs separately
                            // floatingDockContainer == dock means we're dragging the whole tabWidget, so we're leaving nothing behind.
                            if (dockWidget == dock && m_state.tabWidget && m_state.floatingDockContainer != dock)
                            {
                                numVisibleDockWidgets += m_state.tabWidget->count();
                            }
                            // Otherwise just count the single dock widget
                            else
                            {
                                ++numVisibleDockWidgets;
                            }
                        }
                    }

                    if (numVisibleDockWidgets == 1)
                    {
                        singleFloatingDockWidget = containerDockWidget;
                    }
                }
            }
            if (singleFloatingDockWidget)
            {
                // Restore window if maximized when dragging
                if (singleFloatingDockWidget->isMaximized())
                {
                    singleFloatingDockWidget->showNormal();
                }

                singleFloatingDockWidget->hide();
            }
            // Otherwise, we need to hide the original widget while we are dragging
            // around the placeholder. Actual hiding it would minimize the dock
            // window, so instead we need to replace it with an empty QWidget,
            // and save the original content widget so we can restore it later
            else if (m_state.draggedDockWidget)
            {
                m_state.draggedWidget = m_state.draggedDockWidget->widget();
                m_state.draggedDockWidget->setWidget(m_emptyWidget);
                m_emptyWidget->show();
            }

            m_dropZoneState.setDragging(true);
        }

        if (m_dropZoneState.dragging())
        {
            // Don't show dropzones if the window is not dockable
            if (dock->allowedAreas() != Qt::NoDockWidgetArea)
            {
                // Setup the drop zones if there is a valid drop target under the mouse
                QWidget* underMouse = dropWidgetUnderMouse(globalPos, dock);
                setupDropZones(underMouse, globalPos);

                // Store the previous flag for whether or not the cursor is currently
                // over an absolute drop zone so we can compare it later
                bool previousOnAbsoluteDropZone = m_dropZoneState.onAbsoluteDropZone();

                // Check if the mouse is hovered over one of our drop zones
                Qt::DockWidgetArea area = dockAreaForPos(globalPos);

                // If we've hovered over a new drop zone, start our timer to fade in
                // the opacity of the drop zone, which also makes it inactive until
                // the max opacity has been reached
                if (area != Qt::NoDockWidgetArea && (area != m_dropZoneState.dropArea() || previousOnAbsoluteDropZone != m_dropZoneState.onAbsoluteDropZone()))
                {
                    m_dropZoneState.setDropZoneHoverOpacity(0);
                    m_dropZoneHoverFadeInTimer->start();
                }

                SetFloatingPixmapClipping(m_dropZoneState.dropOnto(), area);

                // Save the drop zone area in our drag state
                m_dropZoneState.setDropArea(area);
            }
            else
            {
                m_dropZoneState.setDropArea(Qt::NoDockWidgetArea);
            }

            // Calculate the placeholder rectangle based on the drag position
            QRect dockGeometry = dock->geometry();
            QRect placeholder(dockGeometry
                .translated(globalPos - dock->mapToGlobal(m_state.pressPos))
                .translated(dock->isWindow() ? QPoint() : dock->parentWidget()->mapToGlobal(QPoint())));

            // If we restored the last floating screen grab for this dock widget,
            // then we need to change the placeholder size and update the X coordinate
            // to account for the extrapolated mouse press position
            if (m_state.draggedDockWidget && m_lastFloatingScreenGrab.contains(m_state.draggedDockWidget->objectName()))
            {
                QSize lastFloatingSize = m_state.dockWidgetScreenGrab.size;
                int pressPosX = m_state.pressPos.x();
                int relativeX = (int)(((float)pressPosX / (float)dockGeometry.width()) * ((float)lastFloatingSize.width()));

                placeholder.setSize(lastFloatingSize);
                placeholder.translate(pressPosX - relativeX, 0);
            }

            // Handle snapping to the screen edges/other floating windows while dragging
            QScreen* screen = QApplication::screenAt(globalPos);

            if (screen)
            {
                AdjustForSnapping(placeholder, screen);
                m_state.setPlaceholder(placeholder, screen);

                m_ghostWidget->Enable();
                RepaintFloatingIndicators();
            }
        }

        return m_dropZoneState.dragging();
    }

    void FancyDocking::AdjustForSnapping(QRect& rect, QScreen* cursorScreen)
    {
        m_state.snappedSide = 0;

        // Prevent snapping if any drop zones are present, or if the modifier key
        // is pressed, since that also disables docking
        bool modifiedKeyPressed = FancyDockingDropZoneWidget::CheckModifierKey();
        if (m_dropZoneState.hasDropZones() || m_dropZoneState.absoluteDropZoneRect().isValid() || modifiedKeyPressed)
        {
            return;
        }

        // First, check if we can snap to any of the floating panes
        for (QDockWidget* dockWidget : m_mainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly))
        {
            // Only look at floating windows that aren't hidden
            QWindow* dockWidgetWindow = dockWidget->windowHandle();
            if (dockWidget->isHidden() || !dockWidget->isFloating() || !dockWidgetWindow)
            {
                continue;
            }

            QRect floatingRect = isWin10() ? dockWidgetWindow->geometry() : dockWidgetWindow->frameGeometry();
            AdjustForSnappingToFloatingWindow(rect, floatingRect);
        }

        // Don't continue on if we snapped to a floating pane
        if (m_state.snappedSide != 0)
        {
            return;
        }

        // Next, check if we can snap to the screen edges that the cursor is currently on
        if (AdjustForSnappingToScreenEdges(rect, cursorScreen))
        {
            return;
        }

        // Then, check the rest of the screens
        for (QScreen* screen : QApplication::screens())
        {
            // We already checked this one explicitly first, so move on
            if (screen == cursorScreen)
            {
                continue;
            }

            if (AdjustForSnappingToScreenEdges(rect, screen))
            {
                return;
            }
        }

        // Lastly, check if we can snap to the main editor window
        QWindow* mainWindow = m_mainWindow->window()->windowHandle();
        if (mainWindow)
        {
            QRect mainWindowRect = isWin10() ? mainWindow->geometry() : mainWindow->frameGeometry();
            AdjustForSnappingToFloatingWindow(rect, mainWindowRect);
        }
    }

    bool FancyDocking::AdjustForSnappingToScreenEdges(QRect& rect, QScreen* cursorScreen)
    {
        QRect screenRect = cursorScreen->geometry();
        if (screenRect.isNull())
        {
            return false;
        }

        // Qt returns right/bottom with a -1 offset because of historical reasons,
        // so we need to use x + width instead (but not on Win10)
        int screenRectRight = screenRect.x() + screenRect.width();
        int screenRectBottom = screenRect.y() + screenRect.height();
        if (isWin10())
        {
            screenRectRight -= 1;
            screenRectBottom -= 1;
        }

        // First, check snapping to left/right edges of the screen
        if (qAbs(rect.left() - screenRect.left()) <= g_snapThresholdInPixels)
        {
            rect.moveLeft(screenRect.left());
            m_state.snappedSide |= SnapLeft;
        }
        else if (qAbs(rect.right() - screenRectRight) <= g_snapThresholdInPixels)
        {
            rect.moveRight(screenRectRight);
            m_state.snappedSide |= SnapRight;
        }

        // Then, check shapping to the top/bottom edges of the screen
        if (qAbs(rect.top() - screenRect.top()) <= g_snapThresholdInPixels)
        {
            rect.moveTop(screenRect.top());
            m_state.snappedSide |= SnapTop;
        }
        else if (qAbs(rect.bottom() - screenRectBottom) <= g_snapThresholdInPixels)
        {
            rect.moveBottom(screenRectBottom);
            m_state.snappedSide |= SnapBottom;
        }

        // Return true if we snapped to a screen edge
        return m_state.snappedSide != 0;
    }

    bool FancyDocking::AdjustForSnappingToFloatingWindow(QRect& rect, const QRect& floatingRect)
    {
        QRect currentPlaceholderRect = m_state.placeholder();
        int rectLeft = rect.left();
        int rectRight = rect.x() + rect.width();
        int rectTop = rect.top();
        int rectBottom = rect.y() + rect.height();
        int floatingRectLeft = floatingRect.left();
        int floatingRectRight = floatingRect.x() + floatingRect.width();
        int floatingRectTop = floatingRect.top();
        int floatingRectBottom = floatingRect.y() + floatingRect.height();

        // Qt returns right/bottom with a -1 offset because of historical reasons,
        // so we need to use x + width instead (but not on Win10)
        if (isWin10())
        {
            rectRight -= 1;
            rectBottom -= 1;
            floatingRectRight -= 1;
            floatingRectBottom -= 1;
        }

        // First, check snapping to the left/right edges of the floating window
        // Ensure that we only snap if the placeholder has been dragged within the top/bottom
        // range of the floating window, or if we've already snapped to the top/bottom in the
        // case of snapping left -> left or right -> right
        QRect topBottomRect(floatingRectLeft, rectTop, floatingRect.width(), rect.height());
        bool topOrBottomIntersected = topBottomRect.intersects(floatingRect);
        bool snappedToTopOrBottom = currentPlaceholderRect.top() == floatingRectBottom || currentPlaceholderRect.bottom() == floatingRectTop;
        if ((qAbs(rectLeft - floatingRectLeft) <= g_snapThresholdInPixels) && snappedToTopOrBottom)
        {
            rect.moveLeft(floatingRectLeft);
            m_state.snappedSide |= SnapLeft;
        }
        else if ((qAbs(rectLeft - floatingRectRight) <= g_snapThresholdInPixels) && topOrBottomIntersected)
        {
            rect.moveLeft(floatingRectRight);
            m_state.snappedSide |= SnapLeft;
        }
        else if ((qAbs(rectRight - floatingRectLeft) <= g_snapThresholdInPixels) && topOrBottomIntersected)
        {
            rect.moveRight(floatingRectLeft);
            m_state.snappedSide |= SnapRight;
        }
        else if ((qAbs(rectRight - floatingRectRight) <= g_snapThresholdInPixels) && snappedToTopOrBottom)
        {
            rect.moveRight(floatingRectRight);
            m_state.snappedSide |= SnapRight;
        }

        // Then, check snapping to the top/bottom edges of the floating window
        // Ensure that we only snap if the placeholder has been dragged within the left/right
        // range of the floating window, or if we've already snapped to the left/right in the
        // case of snapping top -> top or bottom -> bottom
        QRect leftRightRect(rectLeft, floatingRectTop, rect.width(), floatingRect.height());
        bool leftOrRightIntersected = leftRightRect.intersects(floatingRect);
        bool snappedToLeftOrRight = currentPlaceholderRect.left() == floatingRectRight || currentPlaceholderRect.right() == floatingRectLeft;
        if ((qAbs(rectTop - floatingRectTop) <= g_snapThresholdInPixels) && snappedToLeftOrRight)
        {
            rect.moveTop(floatingRectTop);
            m_state.snappedSide |= SnapTop;
        }
        else if ((qAbs(rectTop - floatingRectBottom) <= g_snapThresholdInPixels) && leftOrRightIntersected)
        {
            rect.moveTop(floatingRectBottom);
            m_state.snappedSide |= SnapTop;
        }
        else if ((qAbs(rectBottom - floatingRectTop) <= g_snapThresholdInPixels) && leftOrRightIntersected)
        {
            rect.moveBottom(floatingRectTop);
            m_state.snappedSide |= SnapBottom;
        }
        else if ((qAbs(rectBottom - floatingRectBottom) <= g_snapThresholdInPixels) && snappedToLeftOrRight)
        {
            rect.moveBottom(floatingRectBottom);
            m_state.snappedSide |= SnapBottom;
        }

        // Return true if we snapped to a floating window
        return m_state.snappedSide != 0;
    }

    void FancyDocking::RepaintFloatingIndicators()
    {
        updateFloatingPixmap();
    }

    void FancyDocking::SetFloatingPixmapClipping(QWidget* dropOnto, Qt::DockWidgetArea area)
    {
        // If our drop target isn't a main window, then retrieve the main window
        // from the dock widget parent
        QMainWindow* mainWindow = qobject_cast<QMainWindow*>(m_dropZoneState.dropOnto());
        if (!mainWindow && m_dropZoneState.dropOnto())
        {
            mainWindow = qobject_cast<QMainWindow*>(m_dropZoneState.dropOnto()->parentWidget());
        }

        if ((mainWindow == m_mainWindow) && (area != Qt::NoDockWidgetArea) && dropOnto)
        {
            m_ghostWidget->EnableClippingToDockWidgets();
        }
        else
        {
            m_ghostWidget->DisableClippingToDockWidgets();
        }
    }

    /**
     * Handle a mouse press event.
     */
    bool FancyDocking::dockMousePressEvent(QDockWidget* dock, QMouseEvent* event)
    {
        QPoint pressPos = event->pos();
        if (event->button() != Qt::LeftButton || !canDragDockWidget(dock, pressPos))
        {
            return false;
        }

        if (m_state.dock)
        {
            qWarning() << "Press event without the previous being a release?" << dock << m_state.dock;
            return true;
        }

        startDraggingWidget(dock, pressPos);

        // Show the floating pixmap, but don't start it rendering
        // It will early out in it's paint event, but then there
        // won't be any delay when the user has dragged far enough
        // to trigger dragging
        m_ghostWidget->show();

        return true;
    }

    /*
     * Initialize the dragging state for the specified dock widget.  The tabIndex will be -1
     * if you are dragging a regular panel by the title bar, and will be set to a valid index
     * if you are dragging a tab of a DockTabWidget
     */
    void FancyDocking::startDraggingWidget(QDockWidget* dock, const QPoint& pressPos, int tabIndex)
    {
        if (!dock)
        {
            return;
        }

        if (!QApplication::overrideCursor())
        {
            QApplication::setOverrideCursor(m_dragCursor);
        }

        QPoint relativePressPos = pressPos;

        // If we are dragging a floating window, we need to grab a reference to its
        // actual single visible child dock widget to use as our target
        if (dock->isFloating())
        {
            QDockWidget* childDockWidget = nullptr;
            QMainWindow* mainWindow = qobject_cast<QMainWindow*>(dock->widget());
            if (mainWindow)
            {
                for (QDockWidget* dockWidget : mainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly))
                {
                    if (dockWidget->isVisible())
                    {
                        childDockWidget = dockWidget;
                        break;
                    }
                }
            }

            if (!childDockWidget)
            {
                return;
            }

            // Adjust pressPos so that the child widget to be dragged does not change its position.
            relativePressPos = QPoint(pressPos.x(), -(titleBarOffset(dock) - pressPos.y()));

            // Use the visible child as our drag target going forward, and keep a
            // reference to the floating container for decision making later
            m_state.floatingDockContainer = dock;
            dock = childDockWidget;
        }

        if (tabIndex == -1 && m_state.tabWidget)
        {
            // If we're dragging a tab widget by the non tab area, set this so that it gets unpacked properly later.
            m_state.floatingDockContainer = dock;
        }

        QDockWidget* draggedDockWidget = dock;
        m_state.dock = dock;

        // If we are dragging a tab widget, then get a reference to the appropriate widget
        // so we can get the screen grab of just that tab
        if (tabIndex != -1 && m_state.tabWidget)
        {
            QDockWidget* widget = qobject_cast<QDockWidget*>(m_state.tabWidget->widget(tabIndex));
            if (widget)
            {
                draggedDockWidget = widget;
            }
        }

        m_state.draggedDockWidget = draggedDockWidget;

        // If we have cached the last floating screen grab for this dock widget,
        // then retrieve it here, otherwise retrieve a screen grab from the dock
        // widget itself
        QString paneName = draggedDockWidget->objectName();
        if (m_lastFloatingScreenGrab.contains(paneName))
        {
            m_state.dockWidgetScreenGrab = m_lastFloatingScreenGrab[paneName];
        }
        else
        {
            m_state.dockWidgetScreenGrab = { draggedDockWidget->grab(), draggedDockWidget->size() };
        }

        m_state.pressPos = relativePressPos;
        m_dropZoneState.setDragging(false);
        setupDropZones(nullptr);
    }

    bool FancyDocking::dockMouseReleaseEvent(QDockWidget* dock, QMouseEvent* event)
    {
        if (!m_state.dock || event->button() != Qt::LeftButton)
        {
            return false;
        }

        // If we are dragging a floating dock widget, then we need to use the
        // actual dock widget child as our reference
        if (m_state.floatingDockContainer && m_state.floatingDockContainer == dock)
        {
            dock = m_state.dock;
        }

        if (m_dropZoneState.dragging())
        {
            Qt::DockWidgetArea area = m_dropZoneState.dropArea();

            // If the modifier key is pressed, or the hovered drop zone opacity
            // hasn't faded in all the way yet, then ignore the drop zone area
            // which will make the widget floating
            bool modifiedKeyPressed = FancyDockingDropZoneWidget::CheckModifierKey();
            if (modifiedKeyPressed || m_dropZoneState.dropZoneHoverOpacity() != FancyDockingDropZoneConstants::dropZoneOpacity)
            {
                area = Qt::NoDockWidgetArea;
            }

            dropDockWidget(dock, m_dropZoneState.dropOnto(), area);
        }
        else
        {
            // Pass the mouse release event to the tab widget (if applicable) since
            // we grab the mouse/keyboard from it
            if (m_state.tabWidget)
            {
                m_state.tabWidget->mouseReleaseEvent(event);
            }

            clearDraggingState();
        }

        return true;
    }

    /*
     * Handle tab index presses from our DockTabWidgets
     */
    void FancyDocking::onTabIndexPressed(int index)
    {
        DockTabWidget* tabWidget = qobject_cast<DockTabWidget*>(sender());
        if (!tabWidget)
        {
            return;
        }

        QDockWidget* dockWidget = qobject_cast<QDockWidget*>(tabWidget->parent());
        if (!dockWidget)
        {
            return;
        }

        // Initialize our drag state with the dock widget that contains our tab widget
        QPoint pressPos = dockWidget->mapFromGlobal(QCursor::pos());
        m_state.tabWidget = tabWidget;
        startDraggingWidget(dockWidget, pressPos, index);

        // We need to grab the mouse and keyboard immediately because the QTabBar that is part of our
        // DockTabWidget overrides the mouse/key press/move/release events
        m_mainWindow->grabMouse();
        m_mainWindow->grabKeyboard();
    }

    /**
     * Handle tab index presses from our DockTabWidgets so we can delete the tab coutainer if all the tabs are removed
     */
    void FancyDocking::onTabCountChanged(int count)
    {
        // We only care if there are no tabs left
        if (count != 0)
        {
            return;
        }

        // Retrieve the dock widget container for our tab widget
        QDockWidget* dockWidget = getTabWidgetContainer(sender());
        if (!dockWidget)
        {
            return;
        }

        // Retrieve the main window that our dock widget container lives in
        QMainWindow* mainWindow = qobject_cast<QMainWindow*>(dockWidget->parent());
        if (!mainWindow)
        {
            return;
        }

        // Remove the dock widget tab container from the main window and then delete it since
        // it is no longer needed (this will also delete the dock tab widget since it is a child)
        mainWindow->removeDockWidget(dockWidget);
        OptimizedSetParent(dockWidget, nullptr);
        dockWidget->deleteLater();

        // If this tab widget was on a floating window, run the check if this main
        // window needs to be destroyed (if this tab widget was the only thing
        // left in this floating window)
        if (mainWindow != m_mainWindow)
        {
            destroyIfUseless(mainWindow);
        }
    }

    /**
     * Handle tab index changes from our DockTabWidgets so we can update
     * the floating widget's window title based on the current tab name
     */
    void FancyDocking::onCurrentTabIndexChanged(int /*index*/)
    {
        QDockWidget* tabWidgetContainer = getTabWidgetContainer(sender());
        if (!tabWidgetContainer)
        {
            return;
        }

        QMainWindow* mainWindow = qobject_cast<QMainWindow*>(tabWidgetContainer->parentWidget());
        if (mainWindow && mainWindow != m_mainWindow)
        {
            QueueUpdateFloatingWindowTitle(mainWindow);
        }
    }

    /**
     * Whenever widgets are inserted as tabs, cache the tab container they were
     * added to so that if they are closed, we can restore them to the last tab
     * container they were in
     */
    void FancyDocking::onTabWidgetInserted(QWidget* widget)
    {
        if (!widget)
        {
            return;
        }

        // Retrieve the dock widget container for our tab widget
        QDockWidget* dockWidget = getTabWidgetContainer(sender());
        if (!dockWidget)
        {
            return;
        }

        m_lastTabContainerForDockWidget[widget->objectName()] = dockWidget->objectName();
    }

    /**
     * Handle request to undock a tab from a tab group, or undock the entire tab
     * group from its main window
     */
    void FancyDocking::onUndockTab(int index)
    {
        QDockWidget* tabWidgetContainer = getTabWidgetContainer(sender());
        if (!tabWidgetContainer)
        {
            return;
        }

        // If the index given is -1, then we are going to undock the entire tab
        // group, so grab the tab widget container as our target dock widget
        QDockWidget* dockWidget = nullptr;
        if (index == -1)
        {
            dockWidget = tabWidgetContainer;
        }
        // Otherwise, grab the specific dock widget from the tab widget using
        // the specified tab index
        else
        {
            DockTabWidget* tabWidget = qobject_cast<DockTabWidget*>(sender());
            if (!tabWidget)
            {
                return;
            }

            // Set the necessary drag state parameters so that we can undock the
            // given dock widget from the tab widget
            m_state.tabWidget = tabWidget;
            dockWidget = qobject_cast<QDockWidget*>(tabWidget->widget(index));
        }

        undockDockWidget(dockWidget, tabWidgetContainer);
    }

    /**
     * Handle double clicking a tab bar to maximize/restore when the tab bar
     * is shown as a title bar
     */
    void FancyDocking::onTabBarDoubleClicked()
    {
        QDockWidget* tabWidgetContainer = getTabWidgetContainer(sender());
        if (!tabWidgetContainer)
        {
            return;
        }

        QMainWindow* mainWindow = qobject_cast<QMainWindow*>(tabWidgetContainer->parentWidget());
        if (mainWindow && mainWindow != m_mainWindow)
        {
            StyledDockWidget* floatingDockWidget = qobject_cast<StyledDockWidget*>(mainWindow->parentWidget());
            if (floatingDockWidget)
            {
                TitleBar* titleBar = floatingDockWidget->customTitleBar();
                if (titleBar)
                {
                    titleBar->handleMaximize();
                }
            }
        }
    }

    /**
     * Handle request from a dock widget to be undocked from its main window
     */
    void FancyDocking::onUndockDockWidget()
    {
        QDockWidget* dockWidget = qobject_cast<QDockWidget*>(sender());
        undockDockWidget(dockWidget);
    }

    /**
     * Undock the specified dock widget
     */
    void FancyDocking::undockDockWidget(QDockWidget* dockWidget, QDockWidget* placeholder)
    {
        if (!dockWidget)
        {
            return;
        }

        // Offset the geometry that the undocked dock widget will be given from the
        // placeholder geometry with the height of our title dock bar so that it isn't
        // undocked directly above its current position
        int offset = titleBarOffset(dockWidget);

        // The placeholder is an optional parameter to provide a different reference
        // geometry with which to undock the dock widget, so if it isn't provided,
        // then just use our dock widget for reference
        // In practice, if the reference geometry is not provided, that means it's not
        // untabbifying, which means that the title bar will get re-added and/or the size
        // doesn't take it into account, so we need to below otherwise the widget gets smaller
        QSize newSize;
        QPoint newPosition;
        if (!placeholder)
        {
            newSize = dockWidget->size();
            newSize.setHeight(newSize.height() + offset);
            newPosition = dockWidget->mapToGlobal(QPoint(offset, offset));
        }
        else
        {
            newSize = placeholder->size();
            newPosition = placeholder->mapToGlobal(QPoint(offset, offset));
        }

        // Setup the new placeholder using the screen of its new position
        QScreen* screen = Utilities::ScreenAtPoint(newPosition);
        m_state.setPlaceholder(QRect(newPosition, newSize), screen);
        updateFloatingPixmap();

        // Set the widget as being dragged
        m_state.draggedDockWidget = dockWidget;

        // Undock the dock widget
        dropDockWidget(dockWidget, nullptr, Qt::NoDockWidgetArea);
    }

    /**
     * If the specified object is our custom dock tab widget, then return its QDockWidget
     * parent container, otherwise return nullptr
     */
    QDockWidget* FancyDocking::getTabWidgetContainer(QObject* obj)
    {
        // Check if the object is our custom dock tab widget
        DockTabWidget* tabWidget = qobject_cast<DockTabWidget*>(obj);
        if (!tabWidget)
        {
            return nullptr;
        }

        // Retrieve the dock widget container for our tab widget
        return qobject_cast<QDockWidget*>(tabWidget->parent());
    }

    /*
     * Determine whether or not you can drag the specified dock widget based on if the mouse
     * position is inside the title bar
     */
    bool FancyDocking::canDragDockWidget(QDockWidget* dock, QPoint mousePos)
    {
        if (!dock)
        {
            return false;
        }

        QWidget* title = dock->titleBarWidget();
        if (title)
        {
            return title->geometry().contains(mousePos);
        }

        // Some dock widgets don't have a title bar (DockTabWidget and the viewport)
        return false;
    }

    /**
     * Make a dock widget floating by creating a new floating main window containt
     * for it and adding it as the only dock widget
     */
    void FancyDocking::makeDockWidgetFloating(QDockWidget* dock, const QRect& geometry)
    {
        if (!dock)
        {
            return;
        }

        auto styledDockWidget = qobject_cast<StyledDockWidget*>(dock);
        if (styledDockWidget && styledDockWidget->isSingleFloatingChild())
        {
            // Reuse the existing container
            QRect adjustedGeometry = geometry;
            AzQtComponents::EnsureGeometryWithinScreenTop(adjustedGeometry);
            styledDockWidget->window()->setGeometry(adjustedGeometry);
            styledDockWidget->activateWindow();
        }
        else
        {
            // Create a floating window container for this dock widget
            QScopedValueRollback<bool> guard(m_state.updateInProgress, true); // Don't let mainWindow get deleted while we do this
            QMainWindow* mainWindow = createFloatingMainWindow(getUniqueDockWidgetName(m_floatingWindowIdentifierPrefix), geometry, shouldSkipTitleBarOverdraw(dock));
            OptimizedSetParent(dock, mainWindow);
            mainWindow->addDockWidget(Qt::LeftDockWidgetArea, dock);
            dock->show();

            // Make sure we listen for events on the dock widget being put into a floating dock window
            // because this might be called programmatically, so the dock widget might have never been
            // parented to our m_mainWindow initially, so it won't already have an event filter,
            // which will prevent the docking functionality from working.
            dock->installEventFilter(this);
        }
    }

    /**
     * Safe version of the QMainWindow splitDockWidget method to workaround an odd Qt bug
     */
    void FancyDocking::splitDockWidget(QMainWindow* mainWindow, QDockWidget* target, QDockWidget* dropped, Qt::Orientation orientation)
    {
        if (!mainWindow || !target || !dropped)
        {
            return;
        }

        // Calculate the split width (or height) so that our target and dropped
        // widgets can be resized to share the space
        int splitSize = 0;
        if (orientation == Qt::Horizontal)
        {
            splitSize = target->width() / 2;
        }
        else
        {
            splitSize = target->height() / 2;
        }

        // As detailed in LY-42497, there is an odd Qt bug where if dock widget A is
        // already split with dock widget B, and you try to split B with A in the
        // opposite orientation after restoring the QMainWindow state, you will end
        // up with what looks like an empty dock widget in the old location of A,
        // but it's actually a ghost copy in the main window layout of A, which
        // you can tell because it will flicker sometimes and you can see the contents
        // of A.  So to fix, we need to remove the widget being dropped from the main
        // window layout before we split it with the target, and show it afterwards
        // since removing it will also hide it.  This eliminates the ghost copy of
        // the dropped widget that gets left in the main window layout.
        mainWindow->removeDockWidget(dropped);
        mainWindow->splitDockWidget(target, dropped, orientation);
        dropped->show();

        // Resize the target and dropped widgets so they evenly split the space
        // in the orientation that they were split
        mainWindow->resizeDocks({ target, dropped }, { splitSize, splitSize }, orientation);
    }

    /**
    * Use this to prevent the fancy docking system from auto-saving this widget
    * with the rest of the layout state.
    */
    void FancyDocking::disableAutoSaveLayout(QDockWidget* dock)
    {
        dock->setProperty(g_AutoSavePropertyName, false);
    }

    /**
    * Use this to enable the fancy docking system to auto-save this widget
    * with the rest of the layout state.
    *
    * Note that this method does not need to be called in most cases.
    * Unless disableAutoSaveLayout has previously been called, all fancy dock
    * widgets will have their layout state saved.
    */
    void FancyDocking::enableAutoSaveLayout(QDockWidget* dock)
    {
        dock->setProperty(g_AutoSavePropertyName, false);
    }

    bool FancyDocking::IsDockWidgetBeingDragged(QDockWidget* dock)
    {
        return m_state.draggedDockWidget == dock;
    }

    /**
     * Dock a QDockWidget onto a QDockWidget or a QMainWindow
     * NOTE: This method is responsible for calling clearDraggingState() when it has
     * completed its actions
     */
    void FancyDocking::dropDockWidget(QDockWidget* dock, QWidget* onto, Qt::DockWidgetArea area)
    {
        // If the dock widget we are dropping is currently a tab, we need to retrieve it from
        // the tab widget, and remove it as a tab.  We also need to remove its item from our
        // cache of widget <-> tab container since we are moving it somewhere else.
        if (m_state.tabWidget)
        {
            if (m_state.draggedDockWidget)
            {
                m_lastTabContainerForDockWidget.remove(m_state.draggedDockWidget->objectName());
                m_state.tabWidget->removeTab(m_state.draggedDockWidget);
                dock = m_state.draggedDockWidget;
            }
            else
            {
                m_lastTabContainerForDockWidget.remove(dock->objectName());
                m_state.tabWidget->removeTab(dock);
            }
        }

        if (area == Qt::NoDockWidgetArea)
        {
            // Make this dock widget floating, since it has been dropped on no dock area
            // We need to adjust the geometry if it has been snapped to an edge to account
            // for the frame margins, and handle the title bar height offset if it hasn't
            // been snapped to the top edge
            QRect placeholderRect = m_state.placeholder();
            QMargins margins;
            QWindow* mainWindowHandle = m_mainWindow->window()->windowHandle();
            if (mainWindowHandle && !isWin10())
            {
                margins = mainWindowHandle->frameMargins();
            }

            // We also need to account for the window decoration wrapper margins
            // that get added on to floating dock widgets. There is no extra
            // top margin because of the title bar.
            WindowDecorationWrapper* windowWrapper = qobject_cast<WindowDecorationWrapper*>(m_mainWindow->parentWidget()->parentWidget());
            if (windowWrapper)
            {
                QMargins wrapperMargins = windowWrapper->margins();
                margins.setLeft(margins.left() + wrapperMargins.left());
                margins.setRight(margins.right() + wrapperMargins.right());
                margins.setBottom(margins.bottom() + wrapperMargins.bottom());
            }

            // Qt returns right/bottom with a -1 offset because of historical reasons,
            // but even though we ignore this on Win10 when snapping the placeholder,
            // we have to actually put the offset back in before we place it because
            // it ends up being adjusted afterwards
            if (isWin10())
            {
                margins.setRight(margins.right() + 1);
                margins.setBottom(margins.bottom() + 1);
            }

            if (m_state.snappedSide & SnapLeft)
            {
                placeholderRect.translate(margins.left(), 0);
            }
            if (m_state.snappedSide & SnapRight)
            {
                placeholderRect.translate(-margins.right(), 0);
            }
            if (m_state.snappedSide & SnapTop)
            {
                placeholderRect.translate(0, margins.top());
            }
            else
            {
                placeholderRect.adjust(0, -titleBarOffset(dock), 0, 0);
            }
            if (m_state.snappedSide & SnapBottom)
            {
                placeholderRect.translate(0, -margins.bottom());
            }

            // Also adjust the placeholderRect by the relative dpi change from the original screen, since setGeometry uses the screen's
            // virtualGeometry!
            QScreen* fromScreen = dock->screen();
            QScreen* toScreen = Utilities::ScreenAtPoint(placeholderRect.topLeft());

            if (fromScreen != toScreen)
            {
                qreal factorRatio = QHighDpiScaling::factor(fromScreen) / QHighDpiScaling::factor(toScreen);
                placeholderRect.setWidth(aznumeric_cast<int>(aznumeric_cast<qreal>(placeholderRect.width()) * factorRatio));
                placeholderRect.setHeight(aznumeric_cast<int>(aznumeric_cast<qreal>(placeholderRect.height()) * factorRatio));
            }

            // Place the floating dock widget
            makeDockWidgetFloating(dock, placeholderRect);
            clearDraggingState();

            // We can remove any cached floating screen grab for this dock widget
            // now that it's been undocked as floating, since it will be cached
            // whenever it is docked into a main window in the future
            m_lastFloatingScreenGrab.remove(dock->objectName());
        }
        else
        {
            // If we are docking a dock widget that is currently the only dock widget
            // in a floating main window, then cache its screen grab so that we can
            // restore its last floating size when undocking it later in the future
            if (qobject_cast<StyledDockWidget*>(dock)->isSingleFloatingChild())
            {
                m_lastFloatingScreenGrab[dock->objectName()] = m_state.dockWidgetScreenGrab;
            }

            // do the rest after the show has been fully processed, just to be sure
            QTimer::singleShot(0, [=]()
            {
                // Ensure that the dock window is shown, because we may have hidden it when the drag started
                dock->show();

                // Handle an absolute drop zone
                QMainWindow* mainWindow = qobject_cast<QMainWindow*>(onto);
                if (m_dropZoneState.onAbsoluteDropZone())
                {
                    // Find the main window for the drop target (if it's not a main window),
                    // since we will use it instead of the drop target itself for docking on the
                    // absolute edge
                    if (!mainWindow)
                    {
                        mainWindow = qobject_cast<QMainWindow*>(onto->parentWidget());
                    }

                    // Fallback to the editor main window if we couldn't find one
                    if (!mainWindow)
                    {
                        mainWindow = m_mainWindow;
                    }

                    // Set the absolute drop zone corners properly for this
                    // main window
                    setAbsoluteCornersForDockArea(mainWindow, area);
                }

                if (mainWindow)
                {
                    OptimizedSetParent(dock, onto);
                    if (area == Qt::AllDockWidgetAreas)
                    {
                        // should not happen
                    }
                    else
                    {
                        // LY-43595 (similar to LY-42497), there is a bug in Qt where
                        // re-docking a dock widget to different areas in the main
                        // window layout if it was already split in a different
                        // part in the layout results in the dock widget being
                        // duplicated in the layout
                        // We have to show the dock widget after adding it because
                        // the call to removeDockWidget hides the dock widget
                        mainWindow->removeDockWidget(dock);
                        mainWindow->addDockWidget(area, dock, orientation(area));
                        dock->show();
                    }
                }
                else
                {
                    QDockWidget* dockWidget = qobject_cast<QDockWidget*>(onto);
                    if (dockWidget)
                    {
                        mainWindow = static_cast<QMainWindow*>(dockWidget->parentWidget());
                        OptimizedSetParent(dock, mainWindow);
                        if (area == Qt::AllDockWidgetAreas)
                        {
                            tabifyDockWidget(dockWidget, dock, mainWindow, &m_state.dockWidgetScreenGrab);
                        }
                        else
                        {
                            splitDockWidget(mainWindow, dockWidget, dock, orientation(area));
                            if (area == Qt::LeftDockWidgetArea || area == Qt::TopDockWidgetArea)
                            {
                                // Is was actually the other way around that we needed to do.
                                // But we needed the first call so the dock is in the right area.
                                splitDockWidget(mainWindow, dock, dockWidget, orientation(area));
                            }
                        }
                    }
                }

                clearDraggingState();
            });
        }
    }

    /**
     * Dock the dropped dock widget into our custom tab system on the drop target,
     * and return a reference to the tab widget
     */
    DockTabWidget* FancyDocking::tabifyDockWidget(QDockWidget* dropTarget, QDockWidget* dropped, QMainWindow* mainWindow, FancyDocking::WidgetGrab* droppedGrab)
    {
        if (!dropTarget || !dropped || !mainWindow)
        {
            return nullptr;
        }

        // Check if the target dock is already in a tabbed dock
        // If yes, then forward the request to this dock instead.
        {
            if (DockTabWidget::IsTabbed(dropTarget))
            {
                DockTabWidget* tabWidget = DockTabWidget::ParentTabWidget(dropTarget);
                if (QDockWidget* dock = qobject_cast<QDockWidget*>(tabWidget->parentWidget()))
                {
                    return tabifyDockWidget(dock, dropped, mainWindow, droppedGrab);
                }
            }
        }

        // Flag that we have a tabify action in progress so that we can ignore our
        // destroyIfUseless cleanup method that gets inadvertantly triggered
        // while we are tabifying
        QScopedValueRollback<bool> rollback(m_state.updateInProgress, true);

        // Check if the drop target is already one of our custom tab widgets
        DockTabWidget* tabWidget = qobject_cast<DockTabWidget*>(dropTarget->widget());

        QString saveGrabName;
        if (!tabWidget)
        {
            saveGrabName = dropTarget->objectName();
        }
        else if (tabWidget->count() == 1)
        {
            saveGrabName = tabWidget->tabText(0);
        }

        // The dropped dock is already part of this tab widget
        if (tabWidget && tabWidget->indexOf(dropped) != -1)
        {
            return nullptr;
        }

        // Special case this one: if we're dropping onto an untabbed widget, save it's state so that it resizes properly
        // when torn off
        // Should be cleared again when the widget goes back to being a single tab
        if (!m_lastFloatingScreenGrab.contains(saveGrabName))
        {
            m_lastFloatingScreenGrab[saveGrabName] = { dropTarget->grab(), dropTarget->size() };
        }

        if (!tabWidget)
        {
            // The drop target wasn't already a custom tab widget, so create one and replace the drop target
            // with the tab widget (with the drop target as the initial tab)
            tabWidget = createTabWidget(mainWindow, dropTarget);
        }

        // Special case this one: if a widget gets tabbified, when it's untabbified, it won't render properly
        // for the floating pixmap. So we force it to store the state here, if it isn't already
        // It's only if it isn't already, because if it was dragged from a tabgroup and into another tabgroup
        // then we shouldn't be saving it (because it's already been saved)
        QString droppedName = dropped->objectName();
        if (!m_lastFloatingScreenGrab.contains(droppedName) && (droppedGrab != nullptr))
        {
            m_lastFloatingScreenGrab[droppedName] = *droppedGrab;
        }

        // If our dropped widget is also a tab widget (e.g. we dragged a floating tab container),
        // then we need to move the tabs into our drop target tab widget
        int newActiveIndex = 0;
        if (m_state.floatingDockContainer && droppedName.startsWith(m_tabContainerIdentifierPrefix))
        {
            DockTabWidget* oldTabWidget = qobject_cast<DockTabWidget*>(dropped->widget());
            if (!oldTabWidget)
            {
                if (m_state.tabWidget)
                {
                    oldTabWidget = m_state.tabWidget;
                }
                else
                {
                    return tabWidget;
                }
            }

            // Calculate the new active tab index based on adding the tabs to our
            // drop target
            int numOldTabs = oldTabWidget->count();
            newActiveIndex = tabWidget->count() + oldTabWidget->currentIndex();

            // Remove our dropped tabs from their existing tab widget and add them to
            // the drop target tab widget
            for (int i = 0; i < numOldTabs; ++i)
            {
                QDockWidget* dockWidget = qobject_cast<QDockWidget*>(oldTabWidget->widget(0));
                m_lastTabContainerForDockWidget.remove(dockWidget->objectName());
                oldTabWidget->removeTab(0);
                tabWidget->addTab(dockWidget);
            }
        }
        // Otherwise, the dropped widget is a normal dock widget so just add it as
        // a new tab
        else
        {
            newActiveIndex = tabWidget->addTab(dropped);
        }

        // Set the dropped widget as the active tab (or the active tab of the dropped
        // tab widget)
        tabWidget->setCurrentIndex(newActiveIndex);

        return tabWidget;
    }

    /**
     * Reserve the absolute corners for the specified drop zone area for this
     * main window so that any widget docked to that area will take the absolute edge
     */
    void FancyDocking::setAbsoluteCornersForDockArea(QMainWindow* mainWindow, Qt::DockWidgetArea area)
    {
        if (!mainWindow)
        {
            return;
        }

        // Since a widget is being docked on an absolute drop zone,
        // we need to reserve the corners for the absolute drop
        // area so that it will take precedence over other widgets
        // that may already be docked in absolute positions
        switch (area)
        {
        case Qt::LeftDockWidgetArea:
            mainWindow->setCorner(Qt::TopLeftCorner, area);
            mainWindow->setCorner(Qt::BottomLeftCorner, area);
            break;
        case Qt::RightDockWidgetArea:
            mainWindow->setCorner(Qt::TopRightCorner, area);
            mainWindow->setCorner(Qt::BottomRightCorner, area);
            break;
        case Qt::TopDockWidgetArea:
            mainWindow->setCorner(Qt::TopLeftCorner, area);
            mainWindow->setCorner(Qt::TopRightCorner, area);
            break;
        case Qt::BottomDockWidgetArea:
            mainWindow->setCorner(Qt::BottomLeftCorner, area);
            mainWindow->setCorner(Qt::BottomRightCorner, area);
            break;
        }
    }

    bool FancyDocking::eventFilter(QObject* watched, QEvent* event)
    {
        if (watched == m_mainWindow)
        {
            StyledDockWidget* dockWidget = nullptr;
            switch (event->type())
            {
            case QEvent::ChildPolished:
                dockWidget = qobject_cast<StyledDockWidget*>(static_cast<QChildEvent*>(event)->child());
                if (dockWidget)
                {
                    dockWidget->installEventFilter(this);
                    // Remove the movable feature because we will handle that ourselves
                    dockWidget->setFeatures(dockWidget->features() & ~(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable));

                    // Connect to undock requests from this dock widget
                    // MUST BE A UNIQUE CONNECTION! Otherwise, every time through this method will connect to the signal again
                    QObject::connect(dockWidget, &StyledDockWidget::undock, this, &FancyDocking::onUndockDockWidget, Qt::UniqueConnection);
                }
                break;
            case QEvent::MouseMove:
                if (m_state.dock && dockMouseMoveEvent(m_state.dock, static_cast<QMouseEvent*>(event)))
                {
                    return true;
                }
                break;
            case QEvent::MouseButtonRelease:
                if (m_state.dock && dockMouseReleaseEvent(m_state.dock, static_cast<QMouseEvent*>(event)))
                {
                    return true;
                }
                break;
            case QEvent::KeyPress:
            case QEvent::ShortcutOverride:
                if (m_dropZoneState.dragging())
                {
                    // Cancel the dragging state when the Escape key is pressed
                    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
                    if (keyEvent->key() == Qt::Key_Escape)
                    {
                        clearDraggingState();
                    }
                    else
                    {
                        // modifier keys can affect things, so do a redraw
                        RepaintFloatingIndicators();
                    }
                }
                break;
            case QEvent::KeyRelease:
                if (m_dropZoneState.dragging())
                {
                    // modifier keys can affect things, so do a redraw
                    RepaintFloatingIndicators();
                }
                break;
            case QEvent::WindowBlocked:
                // If our main window is deactivated while we are in the middle of
                // a docking drag operation (e.g. popup dialog for new level), we
                // should cancel our drag operation because the mouse release event
                // will be lost since we lost focus
                if (m_dropZoneState.dragging())
                {
                    clearDraggingState();
                }
                break;
            }
        }
        else
        {
            QDockWidget* dockWidget = qobject_cast<QDockWidget*>(watched);
            if (dockWidget)
            {
                QString dockWidgetName = dockWidget->objectName();
                switch (event->type())
                {
                case QEvent::MouseButtonPress:
                    if (dockMousePressEvent(dockWidget, static_cast<QMouseEvent*>(event)))
                    {
                        return true;
                    }
                    break;
                case QEvent::MouseMove:
                    if (dockMouseMoveEvent(dockWidget, static_cast<QMouseEvent*>(event)))
                    {
                        return true;
                    }
                    break;
                case QEvent::MouseButtonRelease:
                    if (dockMouseReleaseEvent(dockWidget, static_cast<QMouseEvent*>(event)))
                    {
                        return true;
                    }
                    break;
                case QEvent::ShowToParent:
                    {
                        QMainWindow* mainWindow = qobject_cast<QMainWindow*>(dockWidget->parent());
                        if (mainWindow)
                        {
                            UpdateTitleBars(mainWindow);

                            if (mainWindow != m_mainWindow)
                            {
                                QueueUpdateFloatingWindowTitle(mainWindow);
                            }
                        }
                    }
                    break;
                case QEvent::HideToParent:
                    {
                        QMainWindow* mainWindow = qobject_cast<QMainWindow*>(dockWidget->parent());
                        if (mainWindow)
                        {
                            UpdateTitleBars(mainWindow);

                            // Update the floating widget's window title
                            if (mainWindow != m_mainWindow)
                            {
                                QueueUpdateFloatingWindowTitle(mainWindow);
                            }

                            // The dockwidget was hidden, we the parent floating mainwindow might need to be
                            // destroyed. But delay the call to destroyIfUseless to the next iteration of the
                            // event loop, as the it might only be temporarily hidden (e.g. reparenting).
                            if (!m_state.updateInProgress)
                            {
                                QTimer::singleShot(0, mainWindow, [this, mainWindow] {
                                    destroyIfUseless(mainWindow);
                                });
                            }
                        }
                    }
                    break;
                case QEvent::Close:
                    // If the user tries to close an entire floating window using
                    // the top title bar, we need to handle the close ourselves
                    if (dockWidgetName.startsWith(m_floatingWindowIdentifierPrefix))
                    {
                        QMainWindow* mainWindow = qobject_cast<QMainWindow*>(dockWidget->widget());
                        if (mainWindow)
                        {
                            // Close the child dock widgets in our floating main window individually so
                            // that they will eventually trigger our destroyIfUseless method, which will
                            // properly save the floating window state in our m_restoreFloatings before
                            // deleting the floating main window, so the next time any of these child
                            // panes are opened, we can re-create the floating main window and restore
                            // them properly
                            for (QDockWidget* childDockWidget : mainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly))
                            {
                                if (childDockWidget->isVisible())
                                {
                                    if (childDockWidget->close())
                                    {
                                        // Destroy any empty container immediately
                                        destroyIfUseless(mainWindow);
                                    }
                                    else
                                    {
                                        // If the child dock widget rejected the close,
                                        // then no need to continue trying to close the
                                        // other children, we can just stop now and ignore
                                        // the close event
                                        event->ignore();
                                        break;
                                    }
                                }
                            }
                            return true;
                        }
                    }
                    break;
                case QEvent::WindowActivate:
                case QEvent::ZOrderChange:
                    // Whenever a floating dock widget is raised to the front, we need
                    // to move it to the front of our z-order list of floating dock widget
                    // names, since Qt doesn't have a way of retrieving the z-order of our
                    // floating dock widgets. The raise can either occur when the user clicks
                    // inside a floating dock widget (WindowActivate), or if the raise() method
                    // is called manually when dragging a dock widget on top of the floating
                    // dock widget (ZOrderChange)
                    if (dockWidgetName.startsWith(m_floatingWindowIdentifierPrefix))
                    {
                        m_orderedFloatingDockWidgetNames.removeAll(dockWidgetName);
                        m_orderedFloatingDockWidgetNames.prepend(dockWidgetName);
                    }
                    break;
                }
            }
            else
            {
                QPointer<QMainWindow> mainWindow = qobject_cast<QMainWindow*>(watched);
                if (mainWindow)
                {
                    QDockWidget* eventChildDockWidget = nullptr;
                    switch (event->type())
                    {
                    case QEvent::ChildAdded:
                    {
                        eventChildDockWidget = qobject_cast<QDockWidget*>(static_cast<QChildEvent*>(event)->child());
                        if (eventChildDockWidget)
                        {
                            QueueUpdateFloatingWindowTitle(mainWindow);
                            UpdateTitleBars(mainWindow);
                        }
                    }
                    break;
                    case QEvent::ChildRemoved:
                    {
                        QueueUpdateFloatingWindowTitle(mainWindow);
                        UpdateTitleBars(mainWindow);

                        SetDragOrDockOnFloatingMainWindow(mainWindow);
                        destroyIfUseless(mainWindow);
                        eventChildDockWidget = qobject_cast<QDockWidget*>(static_cast<QChildEvent*>(event)->child());
                        if (eventChildDockWidget)
                        {
                            // If the dock was deleted, the qobject_cast would fail. So this mean the widget will
                            // be added somewhere else (unless it's not dockable, in which case we'll need to know
                            // that it was last seen in a floating window when it's restored)
                            if (!eventChildDockWidget->objectName().isEmpty() && eventChildDockWidget->allowedAreas() != Qt::NoDockWidgetArea)
                            {
                                m_placeholders.remove(eventChildDockWidget->objectName());
                            }
                        }
                    }
                    break;
                    case QEvent::ChildPolished:
                        // Queue this call since the dock widget won't be visible yet
                        QTimer::singleShot(0, this, [this, mainWindow] {
                            if (mainWindow)
                            {
                                SetDragOrDockOnFloatingMainWindow(mainWindow);
                            }
                        });

                        eventChildDockWidget = qobject_cast<QDockWidget*>(static_cast<QChildEvent*>(event)->child());
                        if (eventChildDockWidget)
                        {
                            if (!eventChildDockWidget->objectName().isEmpty())
                            {
                                m_placeholders[eventChildDockWidget->objectName()] = watched->parent()->objectName();
                            }
                        }
                        break;
                    }
                }
            }
        }

        return false;
    }

    /**
     * If a floating main window has multiple dock widgets, its top title bar should
     * be used for just dragging around to re-position, but if there's only a single
     * dock widget (or single tab widget), then the top title bar should allow
     * the single dock widget to be docked
     */
    void FancyDocking::SetDragOrDockOnFloatingMainWindow(QMainWindow* mainWindow)
    {
        if (!mainWindow)
        {
            return;
        }

        int count = NumVisibleDockWidgets(mainWindow);
        StyledDockWidget* floatingDockWidget = qobject_cast<StyledDockWidget*>(mainWindow->parentWidget());
        if (floatingDockWidget)
        {
            TitleBar* titleBar = floatingDockWidget->customTitleBar();
            if (titleBar)
            {
                bool dragEnabled = (count > 1);

                // If there is only a single dock widget in this floating main window
                // and it has no allowed dockable areas, then set the top title bar
                // be used for dragging to reposition instead of docking
                if (count == 1)
                {
                    QDockWidget* singleDockWidget = mainWindow->findChild<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly);
                    if (singleDockWidget && singleDockWidget->allowedAreas() == Qt::NoDockWidgetArea)
                    {
                        dragEnabled = true;
                    }
                }

                titleBar->setDragEnabled(dragEnabled);
            }
        }
    }

    void FancyDocking::updateFloatingPixmap()
    {
        if (m_dropZoneState.dragging() && m_state.placeholder().isValid())
        {
            bool modifiedKeyPressed = FancyDockingDropZoneWidget::CheckModifierKey();

            m_ghostWidget->setWindowOpacity(modifiedKeyPressed ? 1.0f : FancyDockingDropZoneConstants::draggingDockWidgetOpacity);
            m_ghostWidget->setPixmap(m_state.dockWidgetScreenGrab.screenGrab, m_state.placeholder(), m_state.placeholderScreen());
        }
    }

    void FancyDocking::StartDropZone(QWidget* dropZoneContainer, const QPoint& globalPos)
    {
        // Find any screens that the drop zone container is on
        QList<QScreen*> dropZoneScreens;
        if (dropZoneContainer)
        {
            QRect dropTargetRect = dropZoneContainer->geometry();
            QWidget* dropTargetParent = dropZoneContainer->parentWidget();
            if (dropTargetParent)
            {
                dropTargetRect.moveTopLeft(dropTargetParent->mapToGlobal(dropTargetRect.topLeft()));
            }
            for (QScreen* screen : m_desktopScreens)
            {
                if (dropTargetRect.intersects(screen->geometry()))
                {
                    dropZoneScreens.append(screen);
                }
            }
        }

        // If there's no drop zone target or couldn't find the screen the drop zone
        // target is on, then pick the screen the mouse is currently on so we can
        // have that drop zone widget warmed up
        if (dropZoneScreens.isEmpty())
        {
            for (QScreen* screen : m_desktopScreens)
            {
                if (screen->geometry().contains(globalPos))
                {
                    dropZoneScreens.append(screen);
                    break;
                }
            }
        }

        // Raise any current active drop zone widgets that should still be active
        // and stop any that should no longer be active
        int numActiveDropZoneWidgets = m_activeDropZoneWidgets.size();
        for (int i = 0; i < numActiveDropZoneWidgets; ++i)
        {
            FancyDockingDropZoneWidget* dropZoneWidget = m_activeDropZoneWidgets.takeFirst();
            QScreen* dropZoneScreen = dropZoneWidget->GetScreen();
            if (dropZoneScreens.contains(dropZoneScreen))
            {
                // This screen is already active, so remove it from our list of
                // drop zone screens that need to be activated and raise it
                dropZoneScreens.removeAll(dropZoneScreen);
                dropZoneWidget->raise();

                // Put this drop zone widget back on the end of our active list
                // since we've already processed it
                m_activeDropZoneWidgets.append(dropZoneWidget);
            }
            else
            {
                // Stop this active drop zone widget since it's no longer needed
                dropZoneWidget->Stop();
            }
        }

        // Any screens left aren't active already, so they need to be created if
        // they haven't been already and started
        for (QScreen* screen : dropZoneScreens)
        {
            // Create this drop zone widget if it doesn't already exist, and add
            // it to our list of active drop zone widgets
            FancyDockingDropZoneWidget* dropZoneWidget = m_dropZoneWidgets[screen];
            if (!dropZoneWidget)
            {
                m_dropZoneWidgets[screen] = dropZoneWidget = new FancyDockingDropZoneWidget(m_mainWindow, this, screen, &m_dropZoneState);
            }
            m_activeDropZoneWidgets.append(dropZoneWidget);

            // Start and raise this drop zone widget
            dropZoneWidget->Start();
            dropZoneWidget->raise();
        }

        // floating pixmap is always on top; it'll clip what it's supposed to
        m_ghostWidget->raise();
    }

    void FancyDocking::StopDropZone()
    {
        if (m_activeDropZoneWidgets.size())
        {
            // we have to ensure that we force a repaint, so that there isn't
            // one frame of junk the next time we show the floating drop zones
            for (FancyDockingDropZoneWidget* dropZoneWidget : m_activeDropZoneWidgets)
            {
                dropZoneWidget->repaint();
                dropZoneWidget->Stop();
            }
            m_activeDropZoneWidgets.clear();
        }
    }

    bool FancyDocking::ForceTabbedDocksEnabled() const
    {
        return m_mainWindow->dockOptions() & QMainWindow::ForceTabbedDocks;
    }

    template <typename T>
    T GetProperty(QObject* object, const char* propertyName, const T& returnIfPropertyNotFound)
    {
        QVariant propertyValue = object->property(propertyName);
        if (!propertyValue.isNull() && propertyValue.canConvert<T>())
        {
            return propertyValue.value<T>();
        }

        return returnIfPropertyNotFound;
    }

    /**
     * Analog to QMainWindow::saveState(). The state can be restored with FancyDocking::restoreState()
     */
    QByteArray FancyDocking::saveState()
    {
        SerializedMapType map;
        for (QDockWidget* dockWidget : m_mainWindow->findChildren<QDockWidget*>(
                QRegularExpression(QString("%1.*").arg(m_floatingWindowIdentifierPrefix)), Qt::FindChildrenRecursively))
        {
            QMainWindow* mainWindow = qobject_cast<QMainWindow*>(dockWidget->widget());
            if (!mainWindow)
            {
                continue;
            }
            const auto subs = mainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly);

            // Don't persist any floating windows that have no dock widgets or only have dock widgets that aren't going to be saved (multiple instance panes)
            bool hasValidTab = false;
            for (auto& sub : subs)
            {
                QDockWidget* subDockWidget = qobject_cast<QDockWidget*>(sub);
                if (subDockWidget)
                {
                    bool autoSave = GetProperty(subDockWidget, g_AutoSavePropertyName, true); // we auto-save by default
                    hasValidTab |= autoSave;
                }
            }
            if (!hasValidTab)
            {
                continue;
            }

            QStringList names;
            std::transform(subs.begin(), subs.end(), std::back_inserter(names),
                [](QDockWidget* o) { return o->objectName(); });
            map[dockWidget->objectName()] = qMakePair(names, mainWindow->saveState());

            // Store geometry for this floating dock widget. This could also be stored
            // in the map since we don't need the main window's save state again, but
            // that would involve a new save/restore version
            if (mainWindow != m_mainWindow)
            {
                QString floatingDockWidgetName = dockWidget->objectName();
                if (!floatingDockWidgetName.isEmpty())
                {
                    m_restoreFloatings[floatingDockWidgetName] = qMakePair(mainWindow->saveState(), dockWidget->geometry());
                }
            }
        }

        // Find all of our tab container dock widgets that hold our dock tab widgets
        SerializedTabType tabContainers;
        for (QDockWidget* dockWidget : m_mainWindow->findChildren<QDockWidget*>(
            QRegularExpression(QString("%1.*").arg(m_tabContainerIdentifierPrefix)), Qt::FindChildrenRecursively))
        {
            DockTabWidget* tabWidget = qobject_cast<DockTabWidget*>(dockWidget->widget());
            if (!tabWidget)
            {
                continue;
            }

            // Retrieve the names of dock widgets tabbed inside the tab widget
            // which will be what is matched against when restoring the state
            QStringList tabNames;
            int numTabs = tabWidget->count();
            for (int i = 0; i < numTabs; ++i)
            {
                QWidget* tabbedWidget = tabWidget->widget(i);
                if (tabbedWidget)
                {
                    tabNames.append(tabbedWidget->objectName());
                }
            }

            // Retrieve the main window for the tab widget so that we can see if it
            // is docked in our main window, or in a floating window
            QMainWindow* mainWindow = qobject_cast<QMainWindow*>(dockWidget->parentWidget());
            if (!mainWindow)
            {
                continue;
            }

            // If the tab container is docked in our main window, we will store the
            // floatingDockName as empty.  Otherwise, we need to retrieve the name
            // of the floating dock widget so we can restore the tab container
            // to the appropriate main window.
            QString floatingDockName;
            if (mainWindow != m_mainWindow)
            {
                QDockWidget* floatingDockWidget = qobject_cast<QDockWidget*>(mainWindow->parentWidget());
                if (floatingDockWidget)
                {
                    floatingDockName = floatingDockWidget->objectName();
                }
            }

            // Store this tab container state
            TabContainerType state;
            state.floatingDockName = floatingDockName;
            state.tabNames = tabNames;
            state.currentIndex = tabWidget->currentIndex();
            tabContainers[dockWidget->objectName()] = state;
        }

        QByteArray stateData;
        QDataStream stream(&stateData, QIODevice::WriteOnly);
        stream << quint32(VersionMarker) << m_mainWindow->saveState() << map
        << m_placeholders << m_restoreFloatings << tabContainers;
        return stateData;
    }

    /**
     * Analog to QMainWindow::restoreState(). The state must be created with FancyDocking::saveState()
     */
    bool FancyDocking::restoreState(const QByteArray& state)
    {
        if (state.isEmpty())
        {
            return false;
        }
        QByteArray stateData = state;
        QDataStream stream(&stateData, QIODevice::ReadOnly);

        quint32 version;
        SerializedMapType map;
        SerializedTabType tabContainers;
        QByteArray mainState;
        stream >> version;
        if (stream.status() != QDataStream::Ok || version != VersionMarker)
        {
            return false;
        }
        stream >> mainState >> map;
        if (stream.status() != QDataStream::Ok)
        {
            return false;
        }

        stream >> m_placeholders >> m_restoreFloatings >> tabContainers;

        // First, delete all the current floating window
        for (QDockWidget* dockWidget : m_mainWindow->findChildren<QDockWidget*>(
                QRegularExpression(QString("%1.*").arg(m_floatingWindowIdentifierPrefix)), Qt::FindChildrenRecursively))
        {
            QMainWindow* mainWindow = qobject_cast<QMainWindow*>(dockWidget->widget());
            if (!mainWindow)
            {
                continue;
            }
            for (QDockWidget* subDockWidget : mainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly))
            {
                OptimizedSetParent(subDockWidget, m_mainWindow);
                if (!m_mainWindow->restoreDockWidget(subDockWidget))
                {
                    m_mainWindow->addDockWidget(Qt::LeftDockWidgetArea, subDockWidget);
                }
            }
            delete dockWidget;
        }

        // Untab tabbed dock widgets before restoring, as the restore only works on dock widgets parented directly to the main window
        for (QDockWidget* dockWidget : m_mainWindow->findChildren<QDockWidget*>(
            QRegularExpression(QString("%1.*").arg(m_tabContainerIdentifierPrefix)), Qt::FindChildrenRecursively))
        {
            DockTabWidget* tabWidget = qobject_cast<DockTabWidget*>(dockWidget->widget());
            if (!tabWidget)
            {
                continue;
            }

            // Remove the tabs from the tab widget (we don't actually want to close them, which could delete them at this point)
            int numTabs = tabWidget->count();
            for (int i = 0; i < numTabs; ++i)
            {
                tabWidget->removeTab(0);
            }
        }

        // Restore the floating windows
        QHash<QMainWindow*, QByteArray> floatingMainWindows;
        for (auto it = map.begin(); it != map.end(); ++it)
        {
            QString floatingDockName = it.key();
            QStringList childDockNames = it->first;
            QByteArray floatingState = it->second;

            // Don't restore any floating windows that have no cached dock widgets
            if (childDockNames.size() == 0)
            {
                continue;
            }

            // Since the names of panes could change, we need to make sure at least
            // one of the panes in the floating window still exist, otherwise we would
            // be left with an empty floating window
            if (!AnyDockWidgetsExist(childDockNames))
            {
                continue;
            }

            // Iterate over the child dock widgets to determine their dock widget options; must do this first before creating the floating main window
            QVector<QDockWidget*> childDockWidgets;
            bool skipTitleBarOverdraw = false;
            for (const QString &childName : childDockNames)
            {
                QDockWidget* child = m_mainWindow->findChild<QDockWidget*>(childName, Qt::FindDirectChildrenOnly);
                if (!child)
                {
                    continue;
                }

                // save the children so that we don't have to do a find on the main window again in the next loop
                childDockWidgets.push_back(child);

                skipTitleBarOverdraw = skipTitleBarOverdraw || shouldSkipTitleBarOverdraw(child);
            }

            // Restore geometry for this floating dock widget
            QRect restoredRect;
            auto restoreFloating = m_restoreFloatings.find(floatingDockName);
            if (restoreFloating != m_restoreFloatings.end())
            {
                restoredRect = restoreFloating->second;
                m_restoreFloatings.erase(restoreFloating);
            }

            // reparent and dock the child widgets to the new container now
            QMainWindow* mainWindow = createFloatingMainWindow(floatingDockName, restoredRect, skipTitleBarOverdraw);
            for (auto t = childDockWidgets.begin(); t != childDockWidgets.end(); t++)
            {
                QDockWidget* child = *t;
                OptimizedSetParent(child, mainWindow);
                mainWindow->addDockWidget(Qt::LeftDockWidgetArea, child);
            }

            // Store the floating main window with its state so we can restore them
            // after the tab containers have been restored
            floatingMainWindows[mainWindow] = floatingState;
        }

        // Restore our tab containers (need to set our updateInProgress flag here
        // as well or floating windows that contain tab containers will get
        // deleted inadvertently)
        QScopedValueRollback<bool> rollback(m_state.updateInProgress, true);
        for (auto it = tabContainers.begin(); it != tabContainers.end(); ++it)
        {
            QString tabContainerName = it.key();
            TabContainerType tabState = it.value();
            QString floatingDockName = tabState.floatingDockName;
            QStringList tabNames = tabState.tabNames;
            int currentIndex = tabState.currentIndex;

            // Since the names of panes could change, we need to make sure at least
            // one of the panes in the tab container still exist, otherwise we would
            // be left with an empty tab container
            if (!AnyDockWidgetsExist(tabNames))
            {
                continue;
            }

            // If the floatingDockName is empty, then this tab container is meant
            // for our main window
            QMainWindow* mainWindow = nullptr;
            if (floatingDockName.isEmpty())
            {
                mainWindow = m_mainWindow;
            }
            // Otherwise, we need to find the floating dock widget that was
            // restored previously so we can get a reference to its main window
            else
            {
                QDockWidget* floatingDockWidget = m_mainWindow->findChild<QDockWidget*>(floatingDockName, Qt::FindDirectChildrenOnly);
                if (!floatingDockWidget)
                {
                    continue;
                }

                mainWindow = qobject_cast<QMainWindow*>(floatingDockWidget->widget());
                if (!mainWindow)
                {
                    continue;
                }
            }

            // Create a new tab container and tab widget with the same name as the cached tab container
            // so it will be restored in the same spot in the appropriate main window layout
            DockTabWidget* tabWidget = createTabWidget(mainWindow, nullptr, tabContainerName);

            // Move the dock widgets for the specified tabs into our tab widget
            for (QString name : tabNames)
            {
                // The dock widgets will be restored with the same name in the main window, they just won't
                // be in the proper layout since we have our own custom tab system
                QDockWidget* dockWidget = m_mainWindow->findChild<QDockWidget*>(name);
                if (!dockWidget || dockWidget->allowedAreas() == Qt::NoDockWidgetArea)
                {
                    continue;
                }

                // Move the dock widget into our tab widget
                tabWidget->addTab(dockWidget);
            }

            // Restore the cached active tab index
            tabWidget->setCurrentIndex(currentIndex);
        }

        // Restore the state of our floating main windows after the tab containers have
        // been restored, so that their place in the floating main window layouts will
        // be restored properly.  Also keep track if any of our main window restore
        // calls fail so we can report back our status.
        bool ok = true;
        for (auto it = floatingMainWindows.begin(); it != floatingMainWindows.end(); ++it)
        {
            QMainWindow* mainWindow = it.key();
            QByteArray floatingState = it.value();
            if (!mainWindow->restoreState(floatingState))
            {
                ok = false;
            }
        }

        // Restore the main layout
        if (!m_mainWindow->restoreState(mainState))
        {
            ok = false;
        }

        // If any dock widgets are currently docked to the main window that have
        // docking disabled, this means they were previously docked to the main
        // window (or tabbed) and the isDockable flag was later changed to false,
        // so we need to change them to floating dock widgets
        for (QDockWidget* subDockWidget : m_mainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly))
        {
            if (subDockWidget->allowedAreas() == Qt::NoDockWidgetArea)
            {
                makeDockWidgetFloating(subDockWidget, subDockWidget->geometry());
            }
        }

        // Update the TitleBars for the Main Editor Window and all floating windows
        UpdateTitleBars(m_mainWindow);
        for (auto it = floatingMainWindows.begin(); it != floatingMainWindows.end(); ++it)
        {
            UpdateTitleBars(it.key());
        }

        return ok;
    }

    /**
     * Same as QMainWindow::restoreDockWidget, but extended to checking if it was
     * last in one of our custom tab widgets or floating windows
     */
    bool FancyDocking::restoreDockWidget(QDockWidget* dock)
    {
        if (!dock)
        {
            return false;
        }

        // First, check if this dock widget was last in a tab container
        QString dockObjectName = dock->objectName();
        if (m_lastTabContainerForDockWidget.contains(dockObjectName))
        {
            QString tabDockWidgetName = m_lastTabContainerForDockWidget[dockObjectName];
            QDockWidget* dockWidget = m_mainWindow->findChild<QDockWidget*>(tabDockWidgetName);
            if (dockWidget)
            {
                DockTabWidget* tabWidget = qobject_cast<DockTabWidget*>(dockWidget->widget());
                if (tabWidget)
                {
                    tabWidget->addTab(dock);
                    UpdateTitleBars(qobject_cast<QMainWindow*>(dockWidget->widget()));

                    return true;
                }
            }
        }

        // Then, check if it was last in a floating window
        auto it = m_placeholders.find(dockObjectName);
        if (it != m_placeholders.end())
        {
            // The QDockWidget we try to restore was last seen in a floating QMainWindow.
            QString floatingDockWidgetName = *it;
            QDockWidget* dockWidget = m_mainWindow->findChild<QDockWidget*>(floatingDockWidgetName);
            if (dockWidget)
            {
                // That floating QMainWindow still exist.
                QMainWindow* mainWindow = qobject_cast<QMainWindow*>(dockWidget->widget());
                if (mainWindow)
                {
                    OptimizedSetParent(dock, mainWindow);

                    if (mainWindow->restoreDockWidget(dock))
                    {
                        UpdateTitleBars(mainWindow);
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                    
                }
            }
            else
            {
                // It no longer exists, so we need to re-create the floating main
                // window before restoring the dock widget
                auto it2 = m_restoreFloatings.find(floatingDockWidgetName);
                if (it2 != m_restoreFloatings.end())
                {
                    QMainWindow* mainWindow = createFloatingMainWindow(floatingDockWidgetName, it2->second, true);
                    mainWindow->restoreState(it2->first);
                    OptimizedSetParent(dock, mainWindow);
                    m_restoreFloatings.erase(it2);

                    if (mainWindow->restoreDockWidget(dock))
                    {
                        UpdateTitleBars(mainWindow);
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }
            m_placeholders.erase(it);
        }

        // Fallback to letting our main window try to restore it
        return m_mainWindow->restoreDockWidget(dock);
    }

    /**
     * Clear our dragging state and remove the any drop zones that have been setup
     */
    void FancyDocking::clearDraggingState()
    {
        if (QApplication::overrideCursor())
        {
            QApplication::restoreOverrideCursor();
        }

        m_ghostWidget->hide();

        // Release the mouse and keyboard from our main window since we grab them when we start dragging
        m_mainWindow->releaseMouse();
        m_mainWindow->releaseKeyboard();

        // Restore the dragged widget to its dock widget, and reparent our empty
        // placeholder widget to ourselves so that it will get cleaned up properly
        // We do this outside of the check for m_state.dock since there is a case
        // where the m_state.dock could no longer exist if you had ripped out a
        // single tab which would result in the tab container being destroyed
        if (m_state.draggedDockWidget)
        {
            // If the drag was cancelled before the mouse had actually moved far enough to
            // initiate the drag (manhattan length), then we don't need to restore the actual
            // dock widget contents since they will have remained unchanged
            if (m_state.draggedWidget)
            {
                m_state.draggedDockWidget->setWidget(m_state.draggedWidget);
            }

            m_state.draggedDockWidget = nullptr;
            m_state.draggedWidget = nullptr;
            m_emptyWidget->hide();
            m_emptyWidget->setParent(this);
        }

        // If we hid the floating container of the dragged widget because it was
        // the only visible one, then we need to show it again
        if (m_state.dock)
        {
            QMainWindow* mainWindow = qobject_cast<QMainWindow*>(m_state.dock->parentWidget());
            if (mainWindow && mainWindow != m_mainWindow)
            {
                QDockWidget* containerDockWidget = qobject_cast<QDockWidget*>(mainWindow->parentWidget());
                if (containerDockWidget && containerDockWidget->isFloating() && !containerDockWidget->isVisible())
                {
                    containerDockWidget->show();
                }
            }
        }

        // If we were dragging from a tab widget, make sure to reset its drag state
        if (m_state.tabWidget)
        {
            m_state.tabWidget->finishDrag();
        }

        m_state.dock = nullptr;
        m_dropZoneState.setDragging(false);
        m_state.tabWidget = nullptr;
        m_state.setPlaceholder(QRect(), nullptr);
        m_state.floatingDockContainer = nullptr;
        m_state.snappedSide = 0;

        StopDropZone();
        setupDropZones(nullptr);

        m_ghostWidget->Disable();
    }

    /**
     * Check if at least one of the the specified dock widgets exist on our
     * main window
     */
    bool FancyDocking::AnyDockWidgetsExist(QStringList names)
    {
        for (QString name : names)
        {
            // Consider a tab container dock widget as a success case because
            // these will be created by us when restoring state
            if (name.startsWith(m_tabContainerIdentifierPrefix))
            {
                return true;
            }

            QDockWidget* dockWidget = m_mainWindow->findChild<QDockWidget*>(name, Qt::FindDirectChildrenOnly);
            if (dockWidget)
            {
                return true;
            }
        }

        return false;
    }

    /**
     * Returns the offset which should be used to account for the height of the TitleBar.
     *
     * We want the height of the TitleBar of the QMainWindow that this QDockWidget gets placed into
     * when it is floated, not the TitleBar of the QDockWidget itself.
     */
    int FancyDocking::titleBarOffset(const QDockWidget* dockWidget) const
    {
        if (auto outerDockWidget = qobject_cast<StyledDockWidget*>(dockWidget->parentWidget()->parentWidget()))
        {
            return style()->pixelMetric(QStyle::PM_TitleBarHeight, nullptr, outerDockWidget->customTitleBar());
        }
        else
        {
            // This does not return the perfect value every time because the first time a widget is
            // undocked this function is called before the new QMainWindow and TitleBar have been
            // created. In this case we use the dockWidget titleBarWidget to guess a reasonable
            // value.
            return style()->pixelMetric(QStyle::PM_TitleBarHeight, nullptr, dockWidget->titleBarWidget());
        }
    }

} // namespace AzQtComponents

#include "Components/moc_FancyDocking.cpp"
