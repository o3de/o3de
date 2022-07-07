/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/sort.h>

#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#include <AzQtComponents/Components/Widgets/TabWidgetActionToolBar.h>

#include <QAction>
#include <QActionEvent>
#include <QApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLayout>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSpacerItem>
#include <QStyle>
#include <QStyleOption>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace AzQtComponents
{
    static int g_borderWidth = 1;
    static int g_releaseTabMaxAnimationDuration = 250; // Copied from qtabbar_p.h
    // Will be initialized by TabWidget::loadConfig
    static int g_closeButtonWidth = -1;
    static int g_closeButtonPadding = -1;
    static int g_closeButtonMinTabWidth = -1;
    static int g_toolTipTabWidthThreshold = -1;
    static int g_overflowSpacing = -1;
    static QString g_secondaryStyleClass = QStringLiteral("Secondary");
    static QString g_borderedStyleClass = QStringLiteral("Bordered");
    static QString g_emptyStyleClass = QStringLiteral("Empty");

    TabWidget::TabWidget(QWidget* parent)
        : QTabWidget(parent)
        , m_overflowMenu(new QMenu(this))
        , m_actionToolBarContainer(new TabWidgetActionToolBarContainer(this))
    {
        auto tabBar = new TabBar(this);
        setCustomTabBar(tabBar);
        // Forcing styled background to allow using background-color from QSS
        setAttribute(Qt::WA_StyledBackground, true);

        m_actionToolBarContainer->overflowButton()->setVisible(false);
        setCornerWidget(m_actionToolBarContainer, Qt::TopRightCorner);

        connect(m_actionToolBarContainer->overflowButton(), &QToolButton::clicked, this, &TabWidget::showOverflowMenu);
        connect(m_overflowMenu, &QMenu::aboutToShow, this, &TabWidget::populateMenu);
        connect(this, &TabWidget::currentChanged, this, &TabWidget::resetOverflowMenu);

        AzQtComponents::Style::addClass(this, g_emptyStyleClass);
    }

    TabWidget::TabWidget(TabWidgetActionToolBar* actionToolBar, QWidget* parent)
        : TabWidget(parent)
    {
        setActionToolBar(actionToolBar);
    }

    void TabWidget::applySecondaryStyle(TabWidget* tabWidget, bool bordered)
    {
        Style::addClass(tabWidget, g_secondaryStyleClass);
        Style::addClass(tabWidget->tabBar(), g_secondaryStyleClass);

        if (bordered)
        {
            Style::addClass(tabWidget, g_borderedStyleClass);
            Style::addClass(tabWidget->tabBar(), g_borderedStyleClass);
        }

        if (tabWidget->actionToolBar())
        {
            Style::addClass(tabWidget->actionToolBar(), g_secondaryStyleClass);
        }
    }

    TabWidget::Config TabWidget::loadConfig(QSettings& settings)
    {
        Config config = defaultConfig();
        ConfigHelpers::read<QPixmap>(settings, QStringLiteral("TearIcon"), config.tearIcon);
        ConfigHelpers::read<int>(settings, QStringLiteral("TearIconLeftPadding"), config.tearIconLeftPadding);
        ConfigHelpers::read<int>(settings, QStringLiteral("TabHeight"), config.tabHeight);
        ConfigHelpers::read<int>(settings, QStringLiteral("MinimumTabWidth"), config.minimumTabWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("CloseButtonSize"), config.closeButtonSize);
        ConfigHelpers::read<int>(settings, QStringLiteral("TextRightPadding"), config.textRightPadding);
        ConfigHelpers::read<int>(settings, QStringLiteral("CloseButtonRightPadding"), config.closeButtonRightPadding);
        ConfigHelpers::read<int>(settings, QStringLiteral("CloseButtonMinTabWidth"), config.closeButtonMinTabWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("ToolTipTabWidthThreshold"), config.toolTipTabWidthThreshold);
        ConfigHelpers::read<bool>(settings, QStringLiteral("ShowOverflowMenu"), config.showOverflowMenu);
        ConfigHelpers::read<int>(settings, QStringLiteral("OverflowSpacing"), config.overflowSpacing);

        g_closeButtonPadding = config.closeButtonRightPadding;
        g_closeButtonWidth = config.closeButtonSize;
        g_closeButtonMinTabWidth = config.closeButtonMinTabWidth;
        g_toolTipTabWidthThreshold = config.toolTipTabWidthThreshold;
        g_overflowSpacing = config.overflowSpacing;

        return config;
    }

    TabWidget::Config TabWidget::defaultConfig()
    {
        return {
            QPixmap(),  // TearIcon
            3,          // TearIconLeftPadding
            30,         // TabHeight 28 + 1 (top margin) + 1 (bottom margin)
            16,         // MinimumTabWidth
            16,         // CloseButtonSize
            40,         // TextRightPadding
            4,          // CloseButtonRightPadding
            32,         // CloseButtonMinTabWidth
            96,         // ToolTipTabWidthThreshold
            true,       // ShowOverflowMenu
            24          // OverflowSpacing
        };
    }

    void TabWidget::setCustomTabBar(TabBar* tabBar)
    {
        QTabWidget::setTabBar(tabBar);
        tabBar->setHandleOverflow(m_shouldShowOverflowMenu);
        connect(tabBar, &TabBar::overflowingChanged, this, &TabWidget::setOverflowMenuVisible);
    }

    void TabWidget::setActionToolBar(TabWidgetActionToolBar* actionToolBar)
    {
        m_actionToolBarContainer->setActionToolBar(actionToolBar);
    }

    TabWidgetActionToolBar* TabWidget::actionToolBar() const
    {
        return m_actionToolBarContainer->actionToolBar();
    }

    void TabWidget::setActionToolBarVisible(bool visible)
    {
        m_actionToolBarContainer->setActionToolBarVisible(visible);
    }

    bool TabWidget::isActionToolBarVisible() const
    {
        return m_actionToolBarContainer->isActionToolBarVisible();
    }

    void TabWidget::setOverflowButtonSpacing(bool enable)
    {
        m_spaceOverflowButton = enable;
    }

    void TabWidget::resizeEvent(QResizeEvent* resizeEvent)
    {
        auto tabBarWidget = qobject_cast<TabBar*>(tabBar());

        if (tabBarWidget)
        {
            tabBarWidget->resetOverflow();
        }

        QTabWidget::resizeEvent(resizeEvent);
    }

    void TabWidget::tabInserted(int index)
    {
        Q_UNUSED(index);

        if (count() > 0)
        {
            AzQtComponents::Style::removeClass(this, g_emptyStyleClass);
        }

        resetOverflowMenu();
    }

    void TabWidget::tabRemoved(int index)
    {
        Q_UNUSED(index);

        if (count() == 0)
        {
            AzQtComponents::Style::addClass(this, g_emptyStyleClass);
        }

        resetOverflowMenu();
    }

    void TabWidget::setOverflowMenuVisible(bool visible)
    {
        if (m_shouldShowOverflowMenu)
        {
            m_actionToolBarContainer->overflowButton()->setVisible(visible);
        }
        else
        {
            m_actionToolBarContainer->overflowButton()->setVisible(false);
        }

        if (m_shouldShowOverflowMenu && m_spaceOverflowButton && visible)
        {
            m_actionToolBarContainer->overflowSpacer()->changeSize(g_overflowSpacing, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        }
        else
        {
            m_actionToolBarContainer->overflowSpacer()->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        }

        // Force a Layout refresh to update the spacer properly
        m_actionToolBarContainer->layout()->invalidate();
    }

    void TabWidget::resetOverflowMenu()
    {
        m_overFlowMenuDirty = true;
    }

    void TabWidget::populateMenu()
    {
        if (!m_overFlowMenuDirty)
        {
            return;
        }

        m_overFlowMenuDirty = false;
        m_overflowMenu->clear();

        const int tabCount = count();
        const int currentTab = currentIndex();
        for (int i = 0; i < tabCount; ++i)
        {
            auto action = new QAction(tabText(i));
            action->setCheckable(true);
            action->setChecked(i == currentTab);
            connect(action, &QAction::triggered, this, std::bind(&TabWidget::setCurrentIndex, this, i));
            m_overflowMenu->addAction(action);
        }
    }

    void TabWidget::showOverflowMenu()
    {
        const auto overflowButton = m_actionToolBarContainer->overflowButton();
        const auto position = overflowButton->mapToGlobal(overflowButton->geometry().bottomLeft());
        m_overflowMenu->exec(position);
    }

    bool TabWidget::polish(Style* style, QWidget* widget, const TabWidget::Config& config)
    {
        Q_UNUSED(config);

        if (auto tabWidget = qobject_cast<TabWidget*>(widget))
        {
            style->repolishOnSettingsChange(widget);
            tabWidget->m_shouldShowOverflowMenu = config.showOverflowMenu;
            auto tabBarWidget = qobject_cast<TabBar*>(tabWidget->tabBar());
            if (tabBarWidget)
            {
                tabBarWidget->setHandleOverflow(tabWidget->m_shouldShowOverflowMenu);
            }
            return true;
        }

        return false;
    }

    bool TabWidget::unpolish(Style* style, QWidget* widget, const TabWidget::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        if (auto tabWidget = qobject_cast<TabWidget*>(widget))
        {
            tabWidget->m_shouldShowOverflowMenu = false;
            auto tabBarWidget = qobject_cast<TabBar*>(tabWidget->tabBar());
            if (tabBarWidget)
            {
                tabBarWidget->setHandleOverflow(tabWidget->m_shouldShowOverflowMenu);
            }
            return true;
        }

        return false;
    }

    TabWidgetActionToolBarContainer::TabWidgetActionToolBarContainer(QWidget* parent)
        : QFrame(parent)
        , m_overflowButton(new QToolButton(this))
        , m_overflowSpacer(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed))
    {
        m_overflowButton->setObjectName(QStringLiteral("tabWidgetOverflowButton"));
        m_overflowButton->setPopupMode(QToolButton::InstantPopup);
        m_overflowButton->setAutoRaise(true);
        m_overflowButton->setIcon(QIcon(QStringLiteral(":/stylesheet/img/UI20/dropdown-button-arrow.svg")));
        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins({}); // Defaults to 0
        layout->addWidget(m_overflowButton);

        layout->addSpacerItem(m_overflowSpacer);
    }

    void TabWidgetActionToolBarContainer::setActionToolBar(TabWidgetActionToolBar* actionToolBar)
    {
        if (!actionToolBar || m_actionToolBar == actionToolBar)
        {
            return;
        }

        auto tabWidget = qobject_cast<TabWidget*>(parent());

        if (m_actionToolBar)
        {
            if (tabWidget)
            {
                tabWidget->removeEventFilter(m_actionToolBar);
            }
            m_actionToolBar->hide();
            layout()->removeWidget(m_actionToolBar);
            m_actionToolBar->setParent(nullptr);
            delete m_actionToolBar;
        }

        m_actionToolBar = actionToolBar;
        if (tabWidget)
        {
            tabWidget->installEventFilter(m_actionToolBar);
        }
        layout()->addWidget(m_actionToolBar);
        m_actionToolBar->show();

        // Fix tab order right away for any buttons that may be already on the
        // action toolbar, then again every time actions change on the toolbar
        fixTabOrder();
        connect(actionToolBar, &TabWidgetActionToolBar::actionsChanged, this, &TabWidgetActionToolBarContainer::fixTabOrder);
    }

    void TabWidgetActionToolBarContainer::fixTabOrder()
    {
        auto tabWidget = qobject_cast<TabWidget*>(parent());
        if (!tabWidget)
        {
            return;
        }

        // Get all buttons from the action tool bar and sort them by order
        // of occurence on the layout
        QVector<QPair<int, QWidget*>> buttons;
        for (QToolButton* p : m_actionToolBar->m_actionButtons)
        {
            int idx = m_actionToolBar->layout()->indexOf(p);
            if (idx >= 0 && p) {
                buttons.push_back({idx, p});
            }
        }

        AZStd::sort(buttons.begin(), buttons.end(), [](const QPair<int, QWidget*>& p1, const QPair<int, QWidget*>& p2) {
            return p1.first < p2.first;
        });

        // set tab order: tabbar -> actionbtn1 -> actionbtn2 ...
        for (int i = 0; i < buttons.size(); i++)
        {
            if (i == 0)
            {
                setTabOrder(tabWidget->tabBar(), buttons[i].second);
            }
            else
            {
                setTabOrder(buttons[i-1].second, buttons[i].second);
            }
        }
    }

    void TabWidgetActionToolBarContainer::setActionToolBarVisible(bool visible)
    {
        if (m_actionToolBar && visible == m_actionToolBar->isVisible())
        {
            return;
        }

        // Creating the action toolbar only when strictly necessary, no need to create it if visible is false.
        if (!m_actionToolBar && visible)
        {
            setActionToolBar(new TabWidgetActionToolBar(this));
        }

        if (m_actionToolBar)
        {
            m_actionToolBar->setVisible(visible);
        }
    }

    bool TabWidgetActionToolBarContainer::isActionToolBarVisible() const
    {
        return (m_actionToolBar && m_actionToolBar->isVisible());
    }

    TabBar::TabBar(QWidget* parent)
        : QTabBar(parent)
    {
        connect(this, &TabBar::currentChanged, this, &TabBar::resetOverflow);
        setMouseTracking(true);
        // Set the hover attribute on the tab bar so it receives paint events on
        // a mouse move. The paint handler updates the close button's visibility
        setAttribute(Qt::WA_Hover);
        AzQtComponents::Style::addClass(this, g_emptyStyleClass);

        QIcon icon = QIcon(QStringLiteral(":/Cursors/Grab_release.svg"));
        m_hoverCursor = QCursor(icon.pixmap(16), 5, 2);

        icon = QIcon(QStringLiteral(":/Cursors/Grabbing.svg"));
        m_dragCursor = QCursor(icon.pixmap(16), 5, 2);

        this->setCursor(m_hoverCursor);                                  
    }

    void TabBar::setHandleOverflow(bool handleOverflow)
    {
        m_handleOverflow = handleOverflow;
    }

    bool TabBar::getHandleOverflow() const
    {
        return m_handleOverflow;
    }

    void TabBar::resetOverflow()
    {
        m_overflowing = OverflowUnchecked;
    }

    void TabBar::enterEvent(QEvent* event)
    {
        auto enterEvent = static_cast<QEnterEvent*>(event);
        m_hoveredTab = tabAt(enterEvent->pos());

        QTabBar::enterEvent(event);
    }

    void TabBar::leaveEvent(QEvent* event)
    {
        m_hoveredTab = -1;

        QTabBar::leaveEvent(event);
    }

    void TabBar::mousePressEvent(QMouseEvent* mouseEvent)
    {
        if (mouseEvent->buttons() & Qt::LeftButton)
        {
            if (!QApplication::overrideCursor() || *QApplication::overrideCursor() != m_dragCursor)
            {
                QApplication::setOverrideCursor(m_dragCursor);
            }

            m_lastMousePress = mouseEvent->pos();
        }

        QTabBar::mousePressEvent(mouseEvent);
    }

    void TabBar::mouseMoveEvent(QMouseEvent* mouseEvent)
    {
        if (mouseEvent->buttons() & Qt::LeftButton)
        {
            // When a tab is moved, the actual tab is hidden and a snapshot rendering of the
            // selected tab is moved around. The close button is not explicitly rendered for the
            // moved tab during this operation. We need to make sure not to set it visible again
            // while the tab is moving. This flag makes sure it happens.

            m_movingTab = true;
        }

        m_hoveredTab = tabAt(mouseEvent->pos());

        QTabBar::mouseMoveEvent(mouseEvent);
    }

    void TabBar::mouseReleaseEvent(QMouseEvent* mouseEvent)
    {
        // Ensure we don't reset the cursor in the case of a dummy event being sent from DockTabWidget to trigger the animation.
        Qt::MouseButtons realButtons = QApplication::mouseButtons();
        if (QApplication::overrideCursor() && !(realButtons & Qt::LeftButton))
        {
            QApplication::restoreOverrideCursor();
        }

        if (m_movingTab && !(mouseEvent->buttons() & Qt::LeftButton))
        {
            // When a moving tab is released, there is a short animation to put the moving tab
            // in place before starting rendering again, need to make sure not to explicitly show
            // any close button before that animation is over, because tabRect(i) will
            // not follow the animated tab, and the close button will be shown in the target tab
            // position.
            // This only pauses any custom close button or tool tip visualization logic during the animation.
            // Formula for animation duration taken from QTabBar::mouseReleaseEvent()
            auto animationDuration = (g_releaseTabMaxAnimationDuration * qAbs(mouseEvent->pos().x() - m_lastMousePress.x())) /
                                     tabRect(currentIndex()).width();
            QTimer::singleShot(animationDuration, [this]() {
                m_movingTab = false;
                update();
            });

        }

        QTabBar::mouseReleaseEvent(mouseEvent);
    }

    void TabBar::paintEvent(QPaintEvent* paintEvent)
    {
        overflowIfNeeded();

        // Close buttons are generated after TabBar is constructed.
        // This means we can't initialize their visibility from the constructor,
        // but need to ensure they are in the right state before painting.
        showCloseButtonAt(m_hoveredTab);
        setToolTipIfNeeded(m_hoveredTab);

        QTabBar::paintEvent(paintEvent);
    }

    QSize TabBar::minimumSizeHint() const
    {
        QSize minSizeHint = QTabBar::minimumSizeHint();

        // Set the minimum size hint for the tab bar to 0 to allow TabWidgets
        // to ignore the tab bar when it is being resized.
        // Without this fix, resizing behavior for the tab bar is not triggered.
        minSizeHint.setWidth(0);

        return minSizeHint;
    }

    void TabBar::tabInserted(int /*index*/)
    {
        if (count() > 0)
        {
            AzQtComponents::Style::removeClass(this, g_emptyStyleClass);
        }

        resetOverflow();
    }

    void TabBar::tabRemoved(int /*index*/)
    {
        if (count() == 0)
        {
            AzQtComponents::Style::addClass(this, g_emptyStyleClass);
        }

        resetOverflow();
    }

    void TabBar::overflowIfNeeded()
    {
        // Check if overflow handling is enabled. This also avoids a Qt tab issue that
        // resets the tab bar's scroll offset to 0 when layoutTabs is triggered below
        if (!m_handleOverflow)
        {
            return;
        }

        if (m_overflowing != OverflowUnchecked)
        {
            return;
        }

        // Do the early-out for the TabWidget check before we calculate anything, otherwise for the case where
        // a TabBar is used for the titlebar display of single dock widgets, it will end up executing this logic
        // on every paintEvent since m_overflowing will always be OverflowUnchecked, which is very expensive since
        // it includes two forced layout refreshes
        auto tabWidget = qobject_cast<TabWidget*>(parent());
        if (!tabWidget)
        {
            return;
        }

        // HACK: implicitly reset QTabWidgetPrivate::layoutDirty
        // Force recomputing tab width with default overflow settings
        setIconSize(iconSize());

        int tabsWidth = 0;
        for (int i = 0; i < count(); i++)
        {
            tabsWidth += tabRect(i).width();
        }

        int availableWidth = tabWidget->width();
        if (tabWidget->isActionToolBarVisible())
        {
            availableWidth -= tabWidget->actionToolBar()->width();
        }

        // If overflow spacing is set, deduct it here so that it's also available when not overflowing.
        if (g_overflowSpacing > 0)
        {
            availableWidth -= g_overflowSpacing;
        }

        if (tabsWidth >= availableWidth)
        {
            m_overflowing = Overflowing;
        }
        else
        {
            m_overflowing = NotOverflowing;
        }

        emit overflowingChanged(m_overflowing == Overflowing);

        // HACK: implicitly reset QTabWidgetPrivate::layoutDirty
        // Force recomputing tab width with new overflow settings
        setIconSize(iconSize());
    }

    void TabBar::showCloseButtonAt(int index)
    {
        if (m_movingTab)
        {
            return;
        }

        ButtonPosition closeSide = (ButtonPosition)style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, nullptr, this);
        for (int i = 0; i < count(); i++)
        {
            QWidget* tabBtn = tabButton(i, closeSide);
            
            if (tabBtn)
            {
                bool shouldShow = (i == index);

                int tabWidth = tabRect(i).width();
                shouldShow &= tabWidth >= g_closeButtonMinTabWidth;

                // Need to explicitly move the button if we want to customize its position
                // Don't bother moving it if it won't be shown
                if (shouldShow)
                {
                    QPoint p = tabRect(i).topLeft();

                    p.setX(p.x() + tabRect(i).width() - g_closeButtonPadding - g_closeButtonWidth);
                    p.setY(p.y() + 1 + (tabRect(i).height() - g_closeButtonWidth) / 2);
                    tabBtn->move(p);
                }

                tabBtn->setVisible(shouldShow);

                // Remove tooltips from close buttons so we can see the tab ones
                tabBtn->setToolTip("");
            }
        }
    }

    void TabBar::setToolTipIfNeeded(int index)
    {
        if (!m_handleOverflow)
        {
            return;
        }

        if (m_movingTab)
        {
            return;
        }

        int tabWidth = tabRect(index).width();
        if (tabWidth < g_toolTipTabWidthThreshold)
        {
            setTabToolTip(index, tabText(index));
        }
        else
        {
            setTabToolTip(index, "");
        }
    }

    bool TabBar::polish(Style* style, QWidget* widget, const TabWidget::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        if (auto tabBar = qobject_cast<TabBar*>(widget))
        {
            tabBar->setUsesScrollButtons(false);
            return true;
        }

        return false;
    }

    bool TabBar::unpolish(Style* style, QWidget* widget, const TabWidget::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        if (auto tabBar = qobject_cast<TabBar*>(widget))
        {
            tabBar->setUsesScrollButtons(true);
            return true;
        }

        return false;
    }

    int TabBar::closeButtonSize(const Style* style, const QStyleOption* option, const QWidget* widget, const TabWidget::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(option);
        Q_UNUSED(widget);

        return config.closeButtonSize;
    }

    bool TabBar::drawTabBarTabLabel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const TabWidget::Config& config)
    {
        const QStyleOptionTab* tabOption = qstyleoption_cast<const QStyleOptionTab*>(option);

        if (!tabOption)
        {
            return false;
        }

        const TabBar* tabBar = qobject_cast<const TabBar*>(widget);
        if (!tabBar)
        {
            return false;
        }

        // Tear icon drawing can't be done properly using QSS only because text and image can't be positioned differently
        if (tabBar->isMovable() && (option->state & (QStyle::State_MouseOver | QStyle::State_Sunken)))
        {
            painter->save();
            const QRect tearIconRectangle(
                option->rect.left() + config.tearIconLeftPadding + g_borderWidth,
                option->rect.top() + (option->rect.height() - config.tearIcon.height()) / 2,
                config.tearIcon.width(),
                config.tearIcon.height());
            style->baseStyle()->drawItemPixmap(painter, tearIconRectangle, 0, config.tearIcon);
            painter->restore();
        }

        painter->save();
        if (tabBar->m_useMaxWidth)
        {
            QRect textRect = style->subElementRect(QStyle::SE_TabBarTabText, option, widget);

            // Extend the label to cover more of the tab width
            QSize widthExtension = QSize(config.closeButtonSize, 0);

            if (option->state & QStyle::State_MouseOver)
            {
                widthExtension /= 4;
            }

            textRect.setSize(textRect.size() + widthExtension);
            
            QString labelText = painter->fontMetrics().elidedText(tabOption->text, Qt::ElideRight, textRect.width());
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, labelText);
        }
        else
        {
            style->baseStyle()->drawControl(QStyle::CE_TabBarTabLabel, option, painter, widget);
        }
        painter->restore();

        return true;
    }

    QSize TabBar::sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& contentsSize, const QWidget* widget, const TabWidget::Config& config)
    {
        const QStyleOptionTab* tabOption = qstyleoption_cast<const QStyleOptionTab*>(option);

        if (!tabOption)
        {
            return QSize();
        }

        const TabBar* tabBar = qobject_cast<const TabBar*>(widget);
        if (!tabBar)
        {
            return QSize();
        }

        QSize size = style->baseStyle()->sizeFromContents(type, tabOption, contentsSize, widget);

        size.setHeight(config.tabHeight);

        if (tabBar->m_overflowing != Overflowing || (tabOption->state & QStyle::State_Selected))
        {
            size.setWidth(size.width() + config.textRightPadding);
            if (!tabBar->tabsClosable())
            {
                size -= QSize(config.closeButtonSize + 1, 0);
            }
        }
        else
        {
            auto tabWidget = qobject_cast<TabWidget*>(tabBar->parent());
            if (!tabWidget)
            {
                return {};
            }

            int availableWidth = tabWidget->width() - tabWidget->count() + 1;
            if (tabWidget->isActionToolBarVisible())
            {
                auto toolBarContainer = qobject_cast<QWidget*>(tabWidget->actionToolBar()->parent());
                if (toolBarContainer != nullptr)
                {
                    availableWidth -= toolBarContainer->size().width();
                }
            }

            QStyleOptionTab selectedOptions;
            tabBar->initStyleOption(&selectedOptions, tabBar->currentIndex());
            QSize selectedSize = sizeFromContents(style, type, &selectedOptions, contentsSize, tabBar, config);
            availableWidth -= selectedSize.width();
            size.setWidth(availableWidth / (tabBar->count() - 1));
        }

        return size;
    }

} // namespace AzQtComponents

#include "Components/Widgets/moc_TabWidget.cpp"
