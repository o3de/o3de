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
#include <AzQtComponents/Components/DockBarButton.h>
#include <AzQtComponents/Components/InteractiveWindowGeometryChanger.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>

#include <QFrame>
#include <QPoint>
#include <QPointer>
#include <QTimer>
#endif

class QMouseEvent;
class QMenu;
class QDockWidget;
class QLabel;
class QHBoxLayout;
class QSettings;
class QStyleOption;
class QStackedLayout;

namespace AzQtComponents
{
    class Style;
    class DockTabBar;
    class ElidingLabel;

    /* TitleBar style is now applied from Qt Style Sheets.
     *
     * This can be found in Code/Framework/AzQtComponents/AzQtComponents/Components/Widgets/TitleBar.qss
     */
    class AZ_QT_COMPONENTS_API TitleBar
        : public QFrame
    {
        Q_OBJECT
        Q_PROPERTY(bool drawSideBorders READ drawSideBorders WRITE setDrawSideBorders NOTIFY drawSideBordersChanged)
        Q_PROPERTY(bool drawSimple READ drawSimple WRITE setDrawSimple NOTIFY drawSimpleChanged)
        Q_PROPERTY(bool forceInactive READ forceInactive WRITE setForceInactive NOTIFY forceInactiveChanged)
        Q_PROPERTY(bool tearEnabled READ tearEnabled WRITE setTearEnabled NOTIFY tearEnabledChanged)
        Q_PROPERTY(bool drawAsTabBar READ drawAsTabBar WRITE setDrawAsTabBar NOTIFY drawAsTabBarChanged)
        Q_PROPERTY(QString windowTitleOverride READ windowTitleOverride WRITE setWindowTitleOverride NOTIFY windowTitleOverrideChanged)
        /**
         * Expose the title using a QT property so that test automation can read it
         */
        Q_PROPERTY(QString title READ title)
    public:
        typedef QList<DockBarButton::WindowDecorationButton> WindowDecorationButtons;

        struct Config
        {
            struct TitleBar
            {
                int height = -1;
                int simpleHeight = -1;
                bool appearAsTabBar = false;
            };

            struct Icon
            {
                bool visible = false;
            };

            struct Title
            {
                int indent = -1;
                bool visibleWhenSimple = false;
            };

            struct Buttons
            {
                bool showDividerButtons = false;
                int spacing = -1;
            };

            TitleBar titleBar;
            Icon icon;
            Title title;
            Buttons buttons;
        };

        /*!
         * Loads the button config data from a settings object.
         */
        static Config loadConfig(QSettings& settings);

        /*!
         * Returns default button config data.
         */
        static Config defaultConfig();

        enum TitleBarDrawMode
        {
            Main = 0,
            Simple,
            Hidden
        };

        explicit TitleBar(QWidget* parent = nullptr);
        ~TitleBar();
        bool drawSideBorders() const { return m_drawSideBorders; }
        void setDrawSideBorders(bool);
        bool drawSimple() const { return m_drawMode == TitleBarDrawMode::Simple; }
        bool drawHidden() const { return m_drawMode == TitleBarDrawMode::Hidden; }
        TitleBarDrawMode drawMode() const { return m_drawMode; }
        void setDrawSimple(bool enable);
        void setDrawMode(TitleBarDrawMode drawMode);
        void setDragEnabled(bool);
        void setIsShowingWindowControls(bool show);
        bool tearEnabled() const { return m_tearEnabled; }
        void setTearEnabled(bool);
        bool drawAsTabBar() const { return m_appearAsTabBar; }
        void setDrawAsTabBar(bool);
        QSize sizeHint() const override;
        const QString& windowTitleOverride() const { return m_titleOverride; }
        void setWindowTitleOverride(const QString&);

        /**
         * Sets the titlebar buttons to show.
         * By default shows: | Minimize | Maximize | Close
         *
         * Example:
         *
         * setButtons({ DockBarButton::DividerButton, DockBarButton::MinimizeButton,
         *                        DockBarButton::DividerButton, DockBarButton::MaximizeButton,
         *                        DockBarButton::DividerButton, DockBarButton::CloseButton});
         */
        void setButtons(WindowDecorationButtons);

        void handleClose();
        void handleMaximize();
        void handleMinimize();
        bool hasButton(DockBarButton::WindowDecorationButton buttonType) const;
        bool buttonIsEnabled(DockBarButton::WindowDecorationButton buttonType) const;
        void handleMoveRequest();
        void handleSizeRequest();

        int numButtons() const;
        bool forceInactive() const { return m_forceInactive; }
        void setForceInactive(bool);

        /**
         * For left,right,bottom we use the native Windows border, but for top it's required we add
         * the margin ourselves.
         */
        bool isTopResizeArea(const QPoint& globalPos) const;
        /**
          * These will only return true ever for macOS.
          */
        bool isLeftResizeArea(const QPoint& globalPos) const;
        bool isRightResizeArea(const QPoint& globalPos) const;

        /**
         * The title rect width minus the buttons rect.
         * In local coords.
         */
        QRect draggableRect() const;

        bool event(QEvent* event) override;

        void disableButton(DockBarButton::WindowDecorationButton buttonType);
        void enableButton(DockBarButton::WindowDecorationButton buttonType);

    Q_SIGNALS:
        void undockAction();
        void drawSideBordersChanged(bool drawSideBorders);
        void drawSimpleChanged(bool drawSimple);
        void forceInactiveChanged(bool forceInactive);
        void tearEnabledChanged(bool tearEnabled);
        void drawAsTabBarChanged(bool drawAsTabBar);
        void windowTitleOverrideChanged(const QString& windowTitleOverride);

    protected:
        void mousePressEvent(QMouseEvent* ev) override;
        void mouseReleaseEvent(QMouseEvent* ev) override;
        void mouseMoveEvent(QMouseEvent* ev) override;
        void mouseDoubleClickEvent(QMouseEvent *ev) override;
        void timerEvent(QTimerEvent* ev) override;
        void contextMenuEvent(QContextMenuEvent* ev) override;
        bool eventFilter(QObject* watched, QEvent* event) override;

    protected Q_SLOTS:
        void handleButtonClicked(DockBarButton::WindowDecorationButton type);

    private:
        friend class Style;

        static bool polish(Style* style, QWidget* widget, const Config& config);
        static bool unpolish(Style* style, QWidget* widget, const Config& config);
        static int titleBarHeight(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config, const TabWidget::Config& tabConfig);

        bool usesCustomTopBorderResizing() const;
        void checkEnableMouseTracking();
        QWidget* dockWidget() const;
        bool isInDockWidget() const;
        bool isInFloatingDockWidget() const;
        bool isInDockWidgetWindowGroup() const;
        void updateStandardContextMenu();
        void updateDockedContextMenu();
        void fixEnabled();
        bool isMaximized() const;
        QString title() const;
        void updateTitle();
        void updateTitleBar();
        void setupButtons(bool useDividerButtons = true);
        void setupButtonsHelper(QFrame* container, QHBoxLayout* layout, bool useDividerButtons);
        bool isDragging() const;
        bool isLeftButtonDown() const;
        bool canDragWindow() const;
        bool isResizingWindow() const;
        bool isDraggingWindow() const;
        void resizeWindow(const QPoint& globalPos);
        void dragWindow(const QPoint& globalPos);
        bool isTitleBarForDockWidget() const;
        DockBarButton* findButton(DockBarButton::WindowDecorationButton buttonType) const;

        QStackedLayout* m_stackedLayout = nullptr;
        DockTabBar* m_tabBar = nullptr;
        QWidget* m_firstButton = nullptr;
        QLabel* m_icon = nullptr;
        ElidingLabel* m_label = nullptr;
        bool m_showLabelWhenSimple = true;
        bool m_appearAsTabBar = false;
        bool m_isShowingWindowControls = false;
        QFrame* m_buttonsContainer = nullptr;
        QFrame* m_tabButtonsContainer = nullptr;
        QHBoxLayout* m_buttonsLayout = nullptr;
        QHBoxLayout* m_tabButtonsLayout = nullptr;
        QString m_titleOverride;
        bool m_drawSideBorders = true;
        TitleBarDrawMode m_drawMode = TitleBarDrawMode::Main;
        bool m_dragEnabled = false;
        bool m_tearEnabled = false;
        QPoint m_dragPos;
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::TitleBar::m_buttons': class 'QList<AzQtComponents::DockBarButton::WindowDecorationButton>' needs to have dll-interface to be used by clients of class 'AzQtComponents::TitleBar'
        WindowDecorationButtons m_buttons;
        AZ_POP_DISABLE_WARNING
        bool m_forceInactive = false; // So we can show it inactive in the gallery, for demo purposes
        bool m_autoButtons = false;
        bool m_pendingRepositioning = false;
        bool m_resizingTop = false;
        bool m_resizingRight = false;
        bool m_resizingLeft = false;
        qreal m_relativeDragPos = 0.0;
        qreal m_lastLocalPosX = 0.0;
        QMenu* m_tabsContextMenu = nullptr;
        QMenu* m_windowContextMenu = nullptr;
        QAction* m_restoreMenuAction = nullptr;
        QAction* m_sizeMenuAction = nullptr;
        QAction* m_moveMenuAction = nullptr;
        QAction* m_minimizeMenuAction = nullptr;
        QAction* m_maximizeMenuAction = nullptr;
        QAction* m_closeMenuAction = nullptr;
        QAction* m_closeTabMenuAction = nullptr;
        QAction* m_closeGroupMenuAction = nullptr;
        QAction* m_undockMenuAction = nullptr;
        QAction* m_undockGroupMenuAction = nullptr;

        QWindow* topLevelWindow() const;
        void updateMouseCursor(const QPoint& globalPos);
        bool canResize() const;

        Qt::CursorShape m_originalCursor = Qt::ArrowCursor;
        QTimer m_enableMouseTrackingTimer;

        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::TitleBar::m_interactiveWindowGeometryChanger': class 'QPointer<AzQtComponents::InteractiveWindowGeometryChanger>' needs to have dll-interface to be used by clients of class 'AzQtComponents::TitleBar'
        QPointer<InteractiveWindowGeometryChanger> m_interactiveWindowGeometryChanger;
        AZ_POP_DISABLE_WARNING
    };
} // namespace AzQtComponents

