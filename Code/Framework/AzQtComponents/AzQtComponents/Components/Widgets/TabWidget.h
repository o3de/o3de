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

#include <QAction>
#include <QPushButton>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTabBar>
#include <QTabWidget>
#endif

class QFrame;
class QSettings;
class QSpacerItem;
class QStyleOption;
class QToolButton;

namespace AzQtComponents
{
    class Style;
    class TabBar;
    class TabWidgetActionToolBar;
    class TabWidgetActionToolBarContainer;

     //! A container for other widgets that provides a tab bar to switch between them.
    class AZ_QT_COMPONENTS_API TabWidget
        : public QTabWidget
    {
        Q_OBJECT

        //! Visibility of the action toolbar.
        Q_PROPERTY(bool actionToolBarVisible READ isActionToolBarVisible WRITE setActionToolBarVisible)

    public:
        //! Style configuration for tab widgets.
        struct Config
        {
            QPixmap tearIcon;               //!< The icon shown on the left side of a tab to show it can be dragged. Must be an svg image.
            int tearIconLeftPadding;        //!< Padding between the tear icon and the left side of the tab, in pixels.
            int tabHeight;                  //!< Height of a tab, in pixels.
            int minimumTabWidth;            //!< Minimum size of tabs when shrunk, in pixels.
            int closeButtonSize;            //!< Size of the close button, both width and height, in pixels.
            int textRightPadding;           //!< Padding between the tab text and the close button, in pixels.
            int closeButtonRightPadding;    //!< Padding between the close button and the right side of the tab, in pixels.
            int closeButtonMinTabWidth;     //!< Width threshold below which the close button is not shown, in pixels.
            int toolTipTabWidthThreshold;   //!< Width threshold below which a tooltip is displayed, in pixels.
            bool showOverflowMenu;          //!< Whether an overflow dropdown menu listing the tabs should be displayed on resize.
            int overflowSpacing;            //!< Spacing between the overflow button and the other buttons when the tabs are shrunk, in pixels.
        };

        //! Applies the "Secondary" style class to a TabWidget.
        //! @guidepage uidev-tab-component.html
        static void applySecondaryStyle(TabWidget* tabWidget, bool bordered = true);

        //! Sets the tab widget style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the tab widget.
        static Config loadConfig(QSettings& settings);
        //! Gets the default tab widget style configuration.
        static Config defaultConfig();

        explicit TabWidget(QWidget* parent = nullptr);
        //! Alternate constructor to pre-populate the action toolbar.
        //! @param actionToolBar The action toolbar to show. Implicitly sets actionToolBarEnabled.
        //! @param parent The parent widget.
        TabWidget(TabWidgetActionToolBar* actionToolBar, QWidget* parent = nullptr);

        //! Sets a custom tab bar overriding the default one.
        void setCustomTabBar(TabBar* tabBar);

        //! Adds or replaces the action toolbar of the widget. Implicitly sets actionToolBarEnabled.
        void setActionToolBar(TabWidgetActionToolBar* actionToolBar);
        //! Returns the action toolbar, or nullptr if not set.
        TabWidgetActionToolBar* actionToolBar() const;

        //! Sets the visibility of the action toolbar.
        //! If none is set, it creates and displays a default one.
        void setActionToolBarVisible(bool visible = true);
        //! Returns true if the action toolbar is visible.
        bool isActionToolBarVisible() const;

        //! Enables extra space between the tabs and the action toolbar to allow easier dragging.
        void setOverflowButtonSpacing(bool enable);

        //! Overrides the QTabWidget resizeEvent function to account for tab sizing.
        void resizeEvent(QResizeEvent* resizeEvent) override;

    protected:
        void tabInserted(int index) override;
        void tabRemoved(int index) override;

    private:
        friend class Style;

        TabWidgetActionToolBarContainer* m_actionToolBarContainer = nullptr;
        QMenu* m_overflowMenu = nullptr;
        bool m_overFlowMenuDirty = true;
        bool m_shouldShowOverflowMenu = false;
        bool m_spaceOverflowButton = false;

        void setOverflowMenuVisible(bool visible);
        void resetOverflowMenu();
        void populateMenu();
        void showOverflowMenu();

        // methods used by Style
        static bool polish(Style* style, QWidget* widget, const TabWidget::Config& config);
        static bool unpolish(Style* style, QWidget* widget, const TabWidget::Config& config);
    };

    //! Container for the TabWidget action toolbar.
    class AZ_QT_COMPONENTS_API TabWidgetActionToolBarContainer
        : public QFrame
    {
        Q_OBJECT

    public:
        explicit TabWidgetActionToolBarContainer(QWidget* parent = nullptr);

        //! Returns the overflow button.
        QToolButton* overflowButton() const { return m_overflowButton; }
        //! Returns the overflow spacer.
        QSpacerItem* overflowSpacer() const { return m_overflowSpacer; }
        //! Returns the action toolbar.
        TabWidgetActionToolBar* actionToolBar() const { return m_actionToolBar; }
        //! Adds or replaces the action toolbar of the widget. Implicitly sets actionToolBarEnabled.
        void setActionToolBar(TabWidgetActionToolBar* actionToolBar);
        //! Sets the visibility of the action toolbar.
        //! If no action toolbar is set, creates and displays a default toolbar.
        void setActionToolBarVisible(bool visible);
        //! Returns true if the action toolbar is visible.
        bool isActionToolBarVisible() const;

    private:
        QToolButton* m_overflowButton = nullptr;
        QSpacerItem* m_overflowSpacer = nullptr;
        TabWidgetActionToolBar* m_actionToolBar = nullptr;

        void fixTabOrder();
    };

    //! Allows switching between different content widgets in a TabWidget.
    class AZ_QT_COMPONENTS_API TabBar
        : public QTabBar
    {
        Q_OBJECT

    public:
        //! Sets whether the widget should handle overflow with a custom menu.
        void setHandleOverflow(bool handleOverflow);
        //! Returns whether the widget handles overflow.
        bool getHandleOverflow() const;

        //! Handler to be called after a new tab is added or inserted at position index.
        void tabInserted(int index) override;
        //! Handler to be called after a new tab is removed at position index.
        void tabRemoved(int index) override;

    Q_SIGNALS:
        //! Triggered when the handle overflow settings are changed.
        void overflowingChanged(bool overflowing);

    protected:
        explicit TabBar(QWidget* parent = nullptr);

        void enterEvent(QEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mousePressEvent(QMouseEvent* mouseEvent) override;
        void mouseMoveEvent(QMouseEvent* mouseEvent) override;
        void mouseReleaseEvent(QMouseEvent* mouseEvent) override;
        void paintEvent(QPaintEvent* paintEvent) override;
        QSize minimumSizeHint() const override;
        void SetUseMaxWidth(bool use) { m_useMaxWidth = use; }


    private:
        friend class Style;
        friend class TabWidget;

        enum Overflow {
            OverflowUnchecked,
            NotOverflowing,
            Overflowing
        };

        bool m_handleOverflow = true;
        bool m_useMaxWidth = false;
        Overflow m_overflowing = OverflowUnchecked;
        int m_hoveredTab = -1;
        bool m_movingTab = false;
        QPoint m_lastMousePress;

        QCursor m_dragCursor;
        QCursor m_hoverCursor;

        void resetOverflow();
        void overflowIfNeeded();
        void showCloseButtonAt(int index);
        void setToolTipIfNeeded(int index);

        // methods used by Style
        static bool polish(Style* style, QWidget* widget, const TabWidget::Config& config);
        static bool unpolish(Style* style, QWidget* widget, const TabWidget::Config& config);
        static int closeButtonSize(const Style* style, const QStyleOption* option, const QWidget* widget, const TabWidget::Config& config);
        static bool drawTabBarTabLabel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const TabWidget::Config& config);
        static QSize sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& contentsSize, const QWidget* widget, const TabWidget::Config& config);
    };

} // namespace AzQtComponents
