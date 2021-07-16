/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <QEvent>
#include <QGraphicsView>
#include <QMimeData>
#include <QTimer>
#include <QToolBar>

#include <AzQtComponents/Components/FancyDocking.h>
#include <AzQtComponents/Components/DockTabWidget.h>

#include <GraphCanvas/Widgets/AssetEditorToolbar/AssetEditorToolbar.h>
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorCentralWidget.h>
#include <StaticLib/GraphCanvas/Widgets/GraphCanvasEditor/ui_GraphCanvasEditorCentralWidget.h>

#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorDockWidget.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>

namespace GraphCanvas
{
    /////////////////////////////////////
    // GraphCanvasEditorEmptyDockWidget
    /////////////////////////////////////
    GraphCanvasEditorEmptyDockWidget::GraphCanvasEditorEmptyDockWidget(QWidget* parent)
        : QDockWidget(parent)
        , m_ui(new Ui::GraphCanvasEditorCentralWidget())
    {
        m_ui->setupUi(this);
        setAcceptDrops(true);

        // Because this is the empty visualization. We don't want a title bar.
        setTitleBarWidget(new QWidget(this));
    }
    
    GraphCanvasEditorEmptyDockWidget::~GraphCanvasEditorEmptyDockWidget()
    {        
    }
        
    void GraphCanvasEditorEmptyDockWidget::SetDragTargetText(const AZStd::string& dragTargetString)
    {
        m_ui->dropTarget->setText(dragTargetString.c_str());
    }

    void GraphCanvasEditorEmptyDockWidget::RegisterAcceptedMimeType(const QString& mimeType)
    {
        m_mimeTypes.emplace_back(mimeType);
    }
    
    void GraphCanvasEditorEmptyDockWidget::SetEditorId(const EditorId& editorId)
    {
        AZ_Warning("GraphCanvas", m_editorId == EditorId() || m_editorId == editorId, "Trying to re-use the same Central widget in two different editors.");
        
        m_editorId = editorId;
    }
    
    const EditorId& GraphCanvasEditorEmptyDockWidget::GetEditorId() const
    {
        return m_editorId;
    }

    void GraphCanvasEditorEmptyDockWidget::dragEnterEvent(QDragEnterEvent* enterEvent)
    {
        const QMimeData* mimeData = enterEvent->mimeData();

        m_allowDrop = AcceptsMimeData(mimeData);
        enterEvent->setAccepted(m_allowDrop);
    }

    void GraphCanvasEditorEmptyDockWidget::dragMoveEvent(QDragMoveEvent* moveEvent)
    {
        moveEvent->setAccepted(m_allowDrop);
    }

    void GraphCanvasEditorEmptyDockWidget::dropEvent(QDropEvent* dropEvent)
    {
        const QMimeData* mimeData = dropEvent->mimeData();

        if (m_allowDrop)
        {
            m_initialDropMimeData.clear();

            for (const QString& mimeType : mimeData->formats())
            {
                m_initialDropMimeData.setData(mimeType, mimeData->data(mimeType));
            }

            QPoint dropPosition = dropEvent->pos();
            QPoint globalPosition = mapToGlobal(dropEvent->pos());

            AZ::EntityId graphId;
            AssetEditorRequestBus::EventResult(graphId, GetEditorId(), &AssetEditorRequests::CreateNewGraph);

            // Need to delay this by a frame to ensure that the graphics view is actually
            // resized correctly, otherwise the node will move away from it's initial position.
            QTimer::singleShot(0, [graphId, dropPosition, globalPosition, this]
            {
                AZ::EntityId viewId;
                SceneRequestBus::EventResult(viewId, graphId, &GraphCanvas::SceneRequests::GetViewId);

                QPointF nodePoint;

                QGraphicsView* graphicsView = nullptr;
                GraphCanvas::ViewRequestBus::EventResult(graphicsView, viewId, &GraphCanvas::ViewRequests::AsGraphicsView);

                if (graphicsView)
                {
                    // Then we want to remap that into the GraphicsView, so we can map that into the GraphicsScene.
                    QPoint viewPoint = graphicsView->mapFromGlobal(globalPosition);
                    nodePoint = graphicsView->mapToScene(viewPoint);
                }
                else
                {
                    // If the view doesn't exist, this is fairly malformed, so we can just use the drop position directly.
                    nodePoint = dropPosition;
                }

                AZ::Vector2 scenePos = AZ::Vector2(aznumeric_cast<float>(nodePoint.x()), aznumeric_cast<float>(nodePoint.y()));
                GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::DispatchSceneDropEvent, scenePos, &this->m_initialDropMimeData);
            });
        }
    }

    bool GraphCanvasEditorEmptyDockWidget::AcceptsMimeData(const QMimeData* mimeData) const
    {
        bool retVal = false;

        for (const QString& acceptedType : m_mimeTypes)
        {
            if (mimeData->hasFormat(acceptedType))
            {
                retVal = true;
                break;
            }
        }

        return retVal;
    }

    /////////////////////////////////
    // AssetEditorCentralDockWindow
    /////////////////////////////////

    AssetEditorCentralDockWindow::AssetEditorCentralDockWindow(const EditorId& editorId, const char* saveIdentifier)
        : AzQtComponents::DockMainWindow()
        , m_editorId(editorId)
        , m_fancyDockingManager(new AzQtComponents::FancyDocking(this, AZStd::string::format("%s_CentralDockWindow", saveIdentifier).c_str()))
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setAutoFillBackground(true);

        setDockNestingEnabled(false);
        setTabPosition(Qt::DockWidgetArea::AllDockWidgetAreas, QTabWidget::TabPosition::North);

        // Only allow our docked graphs to be tabbed (can also be floating and/or tabbed)
        setDockOptions((dockOptions() | ForceTabbedDocks));

        QToolBar* toolBar = new QToolBar(this);
        toolBar->addWidget(aznew AssetEditorToolbar(editorId));
        addToolBar(toolBar);

        m_emptyDockWidget = aznew GraphCanvasEditorEmptyDockWidget(this);
        addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, m_emptyDockWidget);

        QObject::connect(qApp, &QApplication::focusChanged, this, &AssetEditorCentralDockWindow::OnFocusChanged);
    }

    AssetEditorCentralDockWindow::~AssetEditorCentralDockWindow()
    {
        delete m_emptyDockWidget;
        m_emptyDockWidget = nullptr;
    }

    GraphCanvasEditorEmptyDockWidget* AssetEditorCentralDockWindow::GetEmptyDockWidget() const
    {
        return m_emptyDockWidget;
    }

    void AssetEditorCentralDockWindow::CreateTabifiedDockWidget(QDockWidget* dockWidget)
    {
        addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, dockWidget);

        // Make the dock widget tabbed and make the added widget the first tab.
        AzQtComponents::DockTabWidget* dockTabWidget = m_fancyDockingManager->createTabWidget(this, dockWidget);
        AzQtComponents::TabWidget::applySecondaryStyle(dockTabWidget);
    }

    void AssetEditorCentralDockWindow::OnEditorOpened(EditorDockWidget* dockWidget)
    {
        QObject::connect(dockWidget, &EditorDockWidget::OnEditorClosed, this, &AssetEditorCentralDockWindow::OnEditorClosed);
        QObject::connect(dockWidget, &QDockWidget::visibilityChanged, this, &AssetEditorCentralDockWindow::UpdateCentralWidget);

        DockWidgetId activeDockWidgetId;
        ActiveEditorDockWidgetRequestBus::EventResult(activeDockWidgetId, m_editorId, &ActiveEditorDockWidgetRequests::GetDockWidgetId);

        EditorDockWidget* activeDockWidget = nullptr;

        if (activeDockWidgetId.IsValid())
        {
            EditorDockWidgetRequestBus::EventResult(activeDockWidget, activeDockWidgetId, &EditorDockWidgetRequests::AsEditorDockWidget);
        }

        if (activeDockWidget && IsDockedInMainWindow(activeDockWidget))
        {
            m_fancyDockingManager->tabifyDockWidget(activeDockWidget, dockWidget, this);
        }
        else
        {

            if (m_emptyDockWidget->isVisible())
            {
                CreateTabifiedDockWidget(dockWidget);
            }
            else
            {
                QDockWidget* leftMostDock = nullptr;
                int minimumPoint = 0;

                for (QDockWidget* testWidget : m_editorDockWidgets)
                {
                    if (IsDockedInMainWindow(testWidget))
                    {
                        QPoint pos = testWidget->pos();

                        if (leftMostDock == nullptr || pos.x() < minimumPoint)
                        {
                            if (pos.x() >= 0)
                            {
                                leftMostDock = testWidget;
                                minimumPoint = pos.x();
                            }
                        }
                    }
                }

                if (leftMostDock)
                {
                    m_fancyDockingManager->tabifyDockWidget(leftMostDock, dockWidget, this);
                }
                else
                {
                    CreateTabifiedDockWidget(dockWidget);
                }
            }
        }

        m_editorDockWidgets.insert(dockWidget);

        dockWidget->show();
        dockWidget->setFocus();
        dockWidget->raise();

        UpdateCentralWidget();
    }

    void AssetEditorCentralDockWindow::OnEditorClosed(EditorDockWidget* dockWidget)
    {
        emit OnEditorClosing(dockWidget);

        m_editorDockWidgets.erase(dockWidget);

        // Handle setting a new active graph if we close the active graph
        DockWidgetId activeDockWidgetId;
        ActiveEditorDockWidgetRequestBus::EventResult(activeDockWidgetId, m_editorId, &ActiveEditorDockWidgetRequests::GetDockWidgetId);
        if (activeDockWidgetId == dockWidget->GetDockWidgetId())
        {
            if (AzQtComponents::DockTabWidget::IsTabbed(dockWidget))
            {
                AzQtComponents::DockTabWidget* tabWidget = AzQtComponents::DockTabWidget::ParentTabWidget(dockWidget);
                AZ_Assert(tabWidget, "Unable to find tab widget"); // The IsTabbed method above checks for ParentTabWidget returning a valid DockTabWidget

                // If the active graph is the last one left in a tabbed widget, then there are no tabs left to focus so set the active graph to nullptr
                int numTabs = tabWidget->count();
                if (numTabs <= 1)
                {
                    ActiveGraphChanged(nullptr);
                }
                // Otherwise, listen for the tab index to change which will be updated once the active tab is closed,
                // so we can set the new active graph to that tab
                else
                {
                    QObject::connect(tabWidget, &QTabWidget::currentChanged, this, &AssetEditorCentralDockWindow::HandleTabWidgetCurrentChanged, Qt::UniqueConnection);
                }
            }
            else
            {
                // If the active graph was floating by itself, just set the active graph to nullptr so that the
                // user can select another graph to be the new active
                ActiveGraphChanged(nullptr);
            }
        }

        UpdateCentralWidget();
    }

    bool AssetEditorCentralDockWindow::CloseAllEditors()
    {
        while (!m_editorDockWidgets.empty())
        {
            auto it = m_editorDockWidgets.begin();
            QDockWidget* dockWidget = *it;
            if (!dockWidget->close())
            {
                return false;
            }
        }

        return true;
    }

    EditorDockWidget* AssetEditorCentralDockWindow::GetEditorDockWidgetByGraphId(const GraphId& graphId) const
    {
        for (EditorDockWidget* dockWidget : m_editorDockWidgets)
        {
            if (dockWidget->GetGraphId() == graphId)
            {
                return dockWidget;
            }
        }

        return nullptr;
    }

    const AZStd::unordered_set<EditorDockWidget*>& AssetEditorCentralDockWindow::GetEditorDockWidgets() const
    {
        return m_editorDockWidgets;
    }

    void AssetEditorCentralDockWindow::OnFocusChanged(QWidget* /*oldFocus*/, QWidget* newFocus)
    {
        if (newFocus != nullptr)
        {
            QObject* parent = newFocus;
            EditorDockWidget* dockWidget = qobject_cast<EditorDockWidget*>(newFocus);

            while (parent != nullptr && dockWidget == nullptr)
            {
                parent = parent->parent();
                dockWidget = qobject_cast<EditorDockWidget*>(parent);
            }

            if (dockWidget)
            {
                ActiveGraphChanged(dockWidget);
            }
        }
    }

    void AssetEditorCentralDockWindow::UpdateCentralWidget()
    {
        if (!m_emptyDockWidget)
        {
            return;
        }

        // Only check this if UpdateCentralWidget was invoked by the visibility changing of one of our dock widgets
        if (sender())
        {
            // If our empty dock widget isn't parented to our main window, that means that the user
            // docked a floating graph to be tabbed with it, so we need to remove it from the tab
            // widget and add it back to our main window as hidden
            QDockWidget* senderDockWidget = qobject_cast<QDockWidget*>(sender());
            if (senderDockWidget && senderDockWidget->isVisible())
            {
                if (m_emptyDockWidget->parentWidget() != this)
                {
                    if (AzQtComponents::DockTabWidget::IsTabbed(m_emptyDockWidget))
                    {
                        AzQtComponents::DockTabWidget* tabWidget = AzQtComponents::DockTabWidget::ParentTabWidget(m_emptyDockWidget);
                        tabWidget->removeTab(m_emptyDockWidget);
                    }

                    addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, m_emptyDockWidget);
                    m_emptyDockWidget->hide();
                }
            }
        }

        bool isMainWindowEmpty = true;

        for (QDockWidget* dockWidget : m_editorDockWidgets)
        {
            if (IsDockedInMainWindow(dockWidget))
            {
                isMainWindowEmpty = false;
                break;
            }
        }

        if (isMainWindowEmpty && !m_emptyDockWidget->isVisible())
        {
            m_emptyDockWidget->show();
        }
        else if (!isMainWindowEmpty && m_emptyDockWidget->isVisible())
        {
            m_emptyDockWidget->hide();
        }
    }

    void AssetEditorCentralDockWindow::ActiveGraphChanged(EditorDockWidget* dockWidget)
    {
        DockWidgetId activeDockWidgetId;
        ActiveEditorDockWidgetRequestBus::EventResult(activeDockWidgetId, m_editorId, &ActiveEditorDockWidgetRequests::GetDockWidgetId);

        GraphCanvas::GraphId activeGraphId;
        EditorDockWidgetRequestBus::EventResult(activeGraphId, activeDockWidgetId, &EditorDockWidgetRequests::GetGraphId);

        GraphId newGraphId;
        if (dockWidget)
        {
            newGraphId = dockWidget->GetGraphId();
        }

        if (activeGraphId == newGraphId)
        {
            return;
        }

        AssetEditorNotificationBus::Event(m_editorId, &AssetEditorNotifications::PreOnActiveGraphChanged);

        if (dockWidget)
        {
            dockWidget->SignalActiveEditor();
        }

        AssetEditorNotificationBus::Event(m_editorId, &AssetEditorNotifications::OnActiveGraphChanged, newGraphId);
        AssetEditorNotificationBus::Event(m_editorId, &AssetEditorNotifications::PostOnActiveGraphChanged);
    }

    bool AssetEditorCentralDockWindow::IsDockedInMainWindow(QDockWidget* dockWidget)
    {
        if (!dockWidget)
        {
            return false;
        }

        // Find which main window this dock widget is parented to, which will either be this
        // CentralDockWindow instance, or a floating main window container
        QMainWindow* parentMainWindow = nullptr;
        QWidget* parent = dockWidget->parentWidget();
        while (parent)
        {
            parentMainWindow = qobject_cast<QMainWindow*>(parent);
            if (parentMainWindow)
            {
                break;
            }

            parent = parent->parentWidget();
        }

        if (parentMainWindow && parentMainWindow == this)
        {
            return true;
        }

        return false;
    }

    void AssetEditorCentralDockWindow::HandleTabWidgetCurrentChanged(int index)
    {
        // This method is invoked in the scenario when the active graph is about to close and it belongs to a tab widget.
        // The new tab widget index is the graph that was switched to after the active tab was closed, so we will set
        // it as the new active graph.
        AzQtComponents::DockTabWidget* tabWidget = qobject_cast<AzQtComponents::DockTabWidget*>(sender());
        AZ_Assert(tabWidget, "Received tab widget changed signal from unknown sender");

        QObject::disconnect(tabWidget, &QTabWidget::currentChanged, this, &AssetEditorCentralDockWindow::HandleTabWidgetCurrentChanged);

        EditorDockWidget* newDockWidget = qobject_cast<EditorDockWidget*>(tabWidget->widget(index));
        ActiveGraphChanged(newDockWidget);
    }
    
#include <StaticLib/GraphCanvas/Widgets/GraphCanvasEditor/moc_GraphCanvasEditorCentralWidget.cpp>
}
