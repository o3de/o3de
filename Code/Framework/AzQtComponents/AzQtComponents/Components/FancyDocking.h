/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/DockMainWindow.h>
#include <AzQtComponents/Components/DockTabWidget.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/FancyDockingDropZoneWidget.h>

#include <QColor>
#include <QMetaType>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QPointer>
#include <QScopedPointer>
#include <QMap>
#include <QVariant>
#include <QScreen>
#include <QSize>
#include <QApplication>
#endif

class QDesktopWidget;
class QTimer;

namespace AzQtComponents
{
    class FancyDockingGhostWidget;
    class FancyDockingDropZoneWidget;

    /**
     * This class implements the Visual Studio docking style for a QMainWindow.
     * To use is, just create an instance of this class and give the QMainWindow as
     * a parent.
     *
     * One should use saveState/restoreState/restoreDockWidget on the instance
     * if this class rather than directly on the QMainWindow.
     */
    class AZ_QT_COMPONENTS_API FancyDocking
        : public QWidget
    {
        Q_OBJECT

    public:

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] mainWindow: the window to apply the fancy docking to
        //! \param[in] identifierPrefix: prefix to use for properties on widgets/windows to flag them
        //!            as owned by this instance of the FancyDocking manager. Can be anything, as long
        //!            as it is unique
        FancyDocking(DockMainWindow* mainWindow, const char* identifierPrefix = "fancydocking");
        ~FancyDocking();

        struct TabContainerType
        {
            QString floatingDockName;
            QStringList tabNames;
            int currentIndex;
        };

        // Grabbing a widget needs both a pixmap and a size, because the size of the pixmap
        // will not take the screen's scaleFactor into account
        struct WidgetGrab
        {
            QPixmap screenGrab;
            QSize size;
        };

        bool restoreState(const QByteArray& layout_data);
        QByteArray saveState();
        bool restoreDockWidget(QDockWidget* dock);
        DockTabWidget* tabifyDockWidget(QDockWidget* dropTarget, QDockWidget* dropped, QMainWindow* mainWindow, WidgetGrab* droppedGrab = nullptr);
        void setAbsoluteCornersForDockArea(QMainWindow* mainWindow, Qt::DockWidgetArea area);
        void makeDockWidgetFloating(QDockWidget* dock, const QRect& geometry);
        void splitDockWidget(QMainWindow* mainWindow, QDockWidget* target, QDockWidget* dropped, Qt::Orientation orientation);

        void disableAutoSaveLayout(QDockWidget* dock);
        void enableAutoSaveLayout(QDockWidget* dock);

        bool IsDockWidgetBeingDragged(QDockWidget* dock);

        DockTabWidget* createTabWidget(QMainWindow* mainWindow, QDockWidget* widgetToReplace, QString name = QString());
    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

        private Q_SLOTS:
        void onTabIndexPressed(int index);
        void onTabCountChanged(int count);
        void onCurrentTabIndexChanged(int index);
        void onTabWidgetInserted(QWidget* widget);
        void onUndockTab(int index);
        void onTabBarDoubleClicked();
        void onUndockDockWidget();
        void updateDockingGeometry();
        void handleScreenAdded(QScreen* screen);
        void handleScreenRemoved(QScreen* screen);
        void onDropZoneHoverFadeInUpdate();

        void updateFloatingPixmap();

    private:
        typedef QMap<QString, QPair<QStringList, QByteArray> > SerializedMapType;
        typedef QMap<QString, TabContainerType> SerializedTabType;

        // Used to save/restore, history as follows:
        //   5001 - Initial version
        //   5002 - Added custom tab containers
        //   5003 - Extended tab restoration to floating windows
        //   5004 - Got rid of single floating dock widgets, all are now floating
        //          main windows so that they get the extra top bar for repositioning
        //          without engaging docking
        //   5005 - Reworked the m_restoreFloatings to handle restoring floating panes
        //          to their previous location properly since now even single floating
        //          panes are within a floating QMainWindow
        enum
        {
            VersionMarker = 5005
        };

        enum SnappedSide
        {
            SnapLeft = 0x1,
            SnapRight = 0x2,
            SnapTop = 0x4,
            SnapBottom = 0x8
        };

        bool dockMousePressEvent(QDockWidget* dock, QMouseEvent* event);
        bool dockMouseMoveEvent(QDockWidget* dock, QMouseEvent* event);
        bool dockMouseReleaseEvent(QDockWidget* dock, QMouseEvent* event);
        bool canDragDockWidget(QDockWidget* dock, QPoint mousePos);
        void startDraggingWidget(QDockWidget* dock, const QPoint& pressPos, int tabIndex = -1);
        Qt::DockWidgetArea dockAreaForPos(const QPoint& globalPos);
        QWidget* dropTargetForWidget(QWidget* widget, const QPoint& globalPos, QWidget* exclude) const;
        QWidget* dropWidgetUnderMouse(const QPoint& globalPos, QWidget* exclude) const;
        QRect getAbsoluteDropZone(QWidget* dock, Qt::DockWidgetArea& area, const QPoint& globalPos = QPoint());
        void setupDropZones(QWidget* dock, const QPoint& globalPos = QPoint());
        void raiseDockWidgets();
        void dropDockWidget(QDockWidget* dock, QWidget* onto, Qt::DockWidgetArea area);
        QMainWindow* createFloatingMainWindow(const QString& name, const QRect& geometry, bool skipTitleBarDrawing = false);
        QString getUniqueDockWidgetName(const QString& prefix);
        void destroyIfUseless(QMainWindow* mw);
        void clearDraggingState();
        QDockWidget* getTabWidgetContainer(QObject* obj);
        void undockDockWidget(QDockWidget* dockWidget, QDockWidget* placeholder = nullptr);
        void SetDragOrDockOnFloatingMainWindow(QMainWindow* mainWindow);
        int NumVisibleDockWidgets(QMainWindow* mainWindow);
        void QueueUpdateFloatingWindowTitle(QMainWindow* mainWindow);

        void AddTitleBarButtons(AzQtComponents::DockTabWidget* tabWidget, AzQtComponents::TitleBar* titleBar);
        void RemoveTitleBarButtons(AzQtComponents::DockTabWidget* tabWidget, AzQtComponents::TitleBar* titleBar = nullptr);
        void UpdateTitleBars(QMainWindow* mainWindow);
        bool IsFloatingDockWidget(QDockWidget* dockWidget);

        void StartDropZone(QWidget* dropZoneContainer, const QPoint& globalPos);
        void StopDropZone();
        bool ForceTabbedDocksEnabled() const;

        void RepaintFloatingIndicators();
        void SetFloatingPixmapClipping(QWidget* dropOnto, Qt::DockWidgetArea area);

        void AdjustForSnapping(QRect& rect, QScreen* cursorScreen);
        bool AdjustForSnappingToScreenEdges(QRect& rect, QScreen* cursorScreen);
        bool AdjustForSnappingToFloatingWindow(QRect& rect, const QRect& floatingRect);

        bool AnyDockWidgetsExist(QStringList names);

        int titleBarOffset(const QDockWidget* dockWidget) const;

        QPoint multiscreenMapFromGlobal(const QPoint& point) const;
        bool WidgetContainsPoint(QWidget* widget, const QPoint& pos) const;

        QMainWindow* m_mainWindow;
        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        QList<QScreen*> m_desktopScreens;

#ifdef AZ_PLATFORM_WINDOWS
        QList<QWidget*> m_perScreenFullScreenWidgets;
#endif

        // An empty QWidget used as a placeholder when dragging a dock window
        // as opposed to creating a new one each time we start dragging a dock widget
        QWidget* m_emptyWidget;

        FancyDockingDropZoneState m_dropZoneState;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

        // When a user hovers over a drop zone, we will fade it in using this timer
        QTimer* m_dropZoneHoverFadeInTimer;

        struct DragState
        {
            QPointer<QDockWidget> dock;
            QPoint pressPos;
            WidgetGrab dockWidgetScreenGrab;  // Save a screen grab of the widget we are dragging so we can paint it as we drag
            QPointer<DockTabWidget> tabWidget;
            QPointer<QWidget> draggedWidget;
            QPointer<QDockWidget> draggedDockWidget;  // This could be different from m_state.dock if the dock widget being dragged is tabbed
            QPointer<QDockWidget> floatingDockContainer;
            bool updateInProgress = false;
            int snappedSide = 0;

            // A setter, so you don't forget to initialize one of them
            void setPlaceholder(const QRect& rect, QScreen* screen)
            {
                m_placeholderScreen = screen;
                m_placeholder = rect;
            }

            // Overload
            void setPlaceholder(const QRect& rect, int screenIndex)
            {
                setPlaceholder(rect, screenIndex == -1 ? nullptr : qApp->screens().at(screenIndex));
            }

            QRect placeholder() const
            {
                return m_placeholder;
            }

            QScreen* placeholderScreen() const
            {
                return m_placeholderScreen;
            }

        private:
            QPointer<QScreen> m_placeholderScreen;
            QRect m_placeholder;
        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        } m_state;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        // map QDockWidget name with its last floating dock.
        QMap<QString, QString> m_placeholders;
        // map floating dock name with it's serialization and the geometry
        QMap<QString, QPair<QByteArray, QRect> > m_restoreFloatings;
        // Map of the last tab container a dock widget was tabbed in (if any)
        QMap<QString, QString> m_lastTabContainerForDockWidget;

        // Map of the last floating screen grab of a dock widget based on its name
        // so we can restore its previous floating size/placeholder image when
        // dragging it around
        QMap<QString, WidgetGrab> m_lastFloatingScreenGrab;

        QScopedPointer<FancyDockingGhostWidget> m_ghostWidget;
        QMap<QScreen*, FancyDockingDropZoneWidget*> m_dropZoneWidgets;
        QList<FancyDockingDropZoneWidget*> m_activeDropZoneWidgets;

        QList<QString> m_orderedFloatingDockWidgetNames;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

        QString m_floatingWindowIdentifierPrefix;
        QString m_tabContainerIdentifierPrefix;

        QCursor m_dragCursor;
    };

} // namespace AzQtComponents

Q_DECLARE_METATYPE(AzQtComponents::FancyDocking::TabContainerType);

