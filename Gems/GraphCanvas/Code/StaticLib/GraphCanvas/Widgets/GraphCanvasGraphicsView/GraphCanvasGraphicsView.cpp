/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <QBoxLayout>
#include <QClipboard>
#include <QDialog>
#include <QMessageBox>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzToolsFramework/UI/Notifications/ToastBus.h>

#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>

#include <GraphCanvas/Components/Bookmarks/BookmarkBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/GraphicsItems/GraphCanvasSceneEventFilter.h>

#include <GraphCanvas/Utils/ConversionUtils.h>

namespace GraphCanvas
{
    ////////////////////////////
    // GraphCanvasGraphicsView
    ////////////////////////////

    GraphCanvasGraphicsView::GraphCanvasGraphicsView(QWidget* parent, bool registerShortcuts)
        : QGraphicsView(parent)
        , m_isDragSelecting(false)
        , m_checkForEdges(false)
        , m_scrollSpeed(0.0f)
        , m_edgePanning(0.0f, 0.0f)
        , m_minZoom(0.1f)
        , m_maxZoom(2.0f)
        , m_checkForDrag(false)
        , m_ignoreValueChange(false)
        , m_reapplyViewParams(false)
        , m_panCountdown(0.0f)
        , m_isEditing(false)
        , m_queuedFocus(nullptr)
    {
        m_viewId = AZ::Entity::MakeId();
        m_notificationsView = AZStd::make_unique<AzToolsFramework::ToastNotificationsView>(this, AZ_CRC(m_viewId.ToString()));

        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
        setDragMode(QGraphicsView::RubberBandDrag);
        setAlignment(Qt::AlignTop | Qt::AlignLeft);
        setCacheMode(QGraphicsView::CacheBackground);

        m_timer.setSingleShot(true);
        m_timer.setInterval(250);
        m_timer.stop();

        m_styleTimer.setSingleShot(true);
        m_styleTimer.setInterval(250);
        m_styleTimer.stop();

        connect(&m_timer, &QTimer::timeout, this, &GraphCanvasGraphicsView::SaveViewParams);
        connect(&m_styleTimer, &QTimer::timeout, this, &GraphCanvasGraphicsView::DisconnectBoundsSignals);

        connect(this, &QGraphicsView::rubberBandChanged, this, &GraphCanvasGraphicsView::OnRubberBandChanged);

        ViewRequestBus::Handler::BusConnect(GetViewId());

        if (registerShortcuts)
        {
            QAction* centerAction = new QAction(this);
            centerAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::DownArrow));

            connect(centerAction, &QAction::triggered, [this]()
            {
                ShowEntireGraph();
            });

            addAction(centerAction);

            QAction* selectAllAction = new QAction(this);
            selectAllAction->setShortcut(QKeySequence(QKeySequence::SelectAll));

            connect(selectAllAction, &QAction::triggered, [this]()
            {
                SelectAll();
            });

            addAction(selectAllAction);

            {
                QAction* selectAllInputAction = new QAction(this);
                selectAllInputAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left));

                connect(selectAllInputAction, &QAction::triggered, [this]()
                {
                    SelectAllRelative(ConnectionType::CT_Input);
                });

                addAction(selectAllInputAction);
            }

            {
                QAction* selectAllOutputAction = new QAction(this);
                selectAllOutputAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right));

                connect(selectAllOutputAction, &QAction::triggered, [this]()
                {
                    SelectAllRelative(ConnectionType::CT_Output);
                });

                addAction(selectAllOutputAction);
            }

            {
                QAction* selectAllOutputAction = new QAction(this);
                selectAllOutputAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up));

                connect(selectAllOutputAction, &QAction::triggered, [this]()
                {
                    SelectConnectedNodes();
                });

                addAction(selectAllOutputAction);
            }

            // Hook up to escape handling
            AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();

            {
                QAction* gotoStartAction = new QAction(this);
                gotoStartAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Left));

                connect(gotoStartAction, &QAction::triggered, [this]()
                {
                    CenterOnStartOfChain();
                });
            }

            {
                QAction* gotoStartAction = new QAction(this);
                gotoStartAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Right));

                connect(gotoStartAction, &QAction::triggered, [this]()
                {
                    CenterOnEndOfChain();
                });
            }

            {
                QAction* gotoStartAction = new QAction(this);
                gotoStartAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Up));

                connect(gotoStartAction, &QAction::triggered, [this]()
                {
                    CenterOnSelection();
                });
            }

            // Ctrl+"0" overview shortcut.
            {
                QAction* keyAction = new QAction(this);
                keyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));

                connect(keyAction, &QAction::triggered, [this]()
                {
                    if (!m_sceneId.IsValid())
                    {
                        // There's no scene.
                        return;
                    }

                    CenterOnArea(GetCompleteArea());
                });

                addAction(keyAction);
            }

            // Ctrl+"+" zoom-in shortcut.
            {
                QAction* keyAction = new QAction(this);
                keyAction->setShortcuts({ QKeySequence(Qt::CTRL + Qt::Key_Plus),
                    QKeySequence(Qt::CTRL + Qt::Key_Equal) });

                connect(keyAction, &QAction::triggered, [this]()
                {
                    ZoomIn();
                });
                addAction(keyAction);
            }

            // Ctrl+"-" zoom-out shortcut.
            {
                QAction* keyAction = new QAction(this);
                keyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus));

                connect(keyAction, &QAction::triggered, [this]()
                {
                    ZoomOut();
                });

                addAction(keyAction);
            }

            // Ctrl+shift+'p' screenshot graph shortcut
            {
                QAction* keyAction = new QAction(this);
                keyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_P));

                connect(keyAction, &QAction::triggered, [this]()
                {
                    ScreenshotSelection();
                });

                addAction(keyAction);
            }

            bool enableDisabling = false;
            AssetEditorSettingsRequestBus::EventResult(enableDisabling, m_editorId, &AssetEditorSettingsRequests::AllowNodeDisabling);

            if (enableDisabling)
            {

                // ctrl+k, ctrl+u enable selection
                {
                    QAction* keyAction = new QAction(this);
                    keyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_K, Qt::CTRL + Qt::Key_U));

                    connect(keyAction, &QAction::triggered, [this]()
                    {
                        EnableSelection();
                    });
                }

                // Ctrl+k, ctrl+c disable selection
                {
                    QAction* keyAction = new QAction(this);
                    keyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_K, Qt::CTRL + Qt::Key_C));

                    connect(keyAction, &QAction::triggered, [this]()
                    {
                        DisableSelection();
                    });
                }

            }
        }

        // Keep the bookmarks outside of bookmarks for now.
        AZStd::vector< Qt::Key > keyIndexes = { Qt::Key_1, Qt::Key_2, Qt::Key_3, Qt::Key_4, Qt::Key_5, Qt::Key_6, Qt::Key_7, Qt::Key_8, Qt::Key_9 };

        for (int i = 0; i < keyIndexes.size(); ++i)
        {
            Qt::Key currentKey = keyIndexes[i];

            QAction* createBookmarkKeyAction = new QAction(this);
            createBookmarkKeyAction->setShortcut(QKeySequence(Qt::CTRL + currentKey));

            connect(createBookmarkKeyAction, &QAction::triggered, [this, i]()
                {
                    if (rubberBandRect().isNull() && !QApplication::mouseButtons() && !m_isEditing)
                    {
                        this->CreateBookmark(i+1);
                    }
                });

            addAction(createBookmarkKeyAction);

            QAction* activateBookmarkKeyAction = new QAction(this);
            activateBookmarkKeyAction->setShortcut(QKeySequence(currentKey));

            connect(activateBookmarkKeyAction, &QAction::triggered, [this, i]()
                {
                    if (rubberBandRect().isNull() && !QApplication::mouseButtons() && !QApplication::keyboardModifiers() && !m_isEditing)
                    {
                        this->JumpToBookmark(i+1);
                    }
                });

            addAction(activateBookmarkKeyAction);
        }
    }

    GraphCanvasGraphicsView::~GraphCanvasGraphicsView()
    {
        ViewRequestBus::Handler::BusDisconnect();
        SceneNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        AssetEditorSettingsNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();

        ClearScene();
    }

    const ViewId& GraphCanvasGraphicsView::GetViewId() const
    {
        return m_viewId;
    }

    void GraphCanvasGraphicsView::SetEditorId(const EditorId& editorId)
    {
        m_editorId = editorId;

        if (m_sceneId.IsValid())
        {
            SceneRequestBus::Event(m_sceneId, &SceneRequests::SetEditorId, m_editorId);
        }

        AssetEditorSettingsNotificationBus::Handler::BusDisconnect();
        AssetEditorSettingsNotificationBus::Handler::BusConnect(editorId);

        OnSettingsChanged();
    }

    EditorId GraphCanvasGraphicsView::GetEditorId() const
    {
        return m_editorId;
    }

    void GraphCanvasGraphicsView::SetScene(const AZ::EntityId& sceneId)
    {
        if (!sceneId.IsValid())
        {
            return;
        }

        if (SceneRequestBus::FindFirstHandler(sceneId) == nullptr)
        {
            AZ_Assert(false, "Couldn't find the Scene component on entity with ID %s", sceneId.ToString().data());
            return;
        }

        if (m_sceneId == sceneId)
        {
            return;
        }

        ClearScene();
        m_sceneId = sceneId;

        QGraphicsScene* graphicsScene = nullptr;
        SceneRequestBus::EventResult(graphicsScene, m_sceneId, &SceneRequests::AsQGraphicsScene);
        setScene(graphicsScene);

        CalculateMinZoomBounds();

        SceneNotificationBus::Handler::BusConnect(m_sceneId);

        SceneRequestBus::Event(m_sceneId, &SceneRequests::RegisterView, GetViewId());

        ConnectBoundsSignals();
        OnBoundsChanged();
    }

    AZ::EntityId GraphCanvasGraphicsView::GetScene() const
    {
        return m_sceneId;
    }

    void GraphCanvasGraphicsView::ClearScene()
    {
        if (m_sceneId.IsValid())
        {
            SceneRequestBus::Event(m_sceneId, &SceneRequests::RemoveView, GetViewId());
            SceneNotificationBus::Handler::BusDisconnect(m_sceneId);
        }

        m_sceneId.SetInvalid();
        setScene(nullptr);
    }

    AZ::Vector2 GraphCanvasGraphicsView::GetViewSceneCenter() const
    {
        AZ::Vector2 viewCenter(0, 0);
        QPointF centerPoint = mapToScene(rect()).boundingRect().center();

        viewCenter.SetX(aznumeric_cast<float>(centerPoint.x()));
        viewCenter.SetY(aznumeric_cast<float>(centerPoint.y()));

        return viewCenter;
    }

    AZ::Vector2 GraphCanvasGraphicsView::MapToGlobal(const AZ::Vector2& scenePoint)
    {
        QPoint mapped = mapToGlobal(mapFromScene(ConversionUtils::AZToQPoint(scenePoint)));
        return ConversionUtils::QPointToVector(mapped);
    }

    AZ::Vector2 GraphCanvasGraphicsView::MapToScene(const AZ::Vector2& view)
    {
        QPointF mapped = mapToScene(ConversionUtils::AZToQPoint(view).toPoint());
        return ConversionUtils::QPointToVector(mapped);
    }

    AZ::Vector2 GraphCanvasGraphicsView::MapFromScene(const AZ::Vector2& scene)
    {
        QPoint mapped = mapFromScene(ConversionUtils::AZToQPoint(scene));
        return ConversionUtils::QPointToVector(mapped);
    }

    void GraphCanvasGraphicsView::SetViewParams(const ViewParams& viewParams)
    {
        // Fun explanation time.
        //
        // The GraphicsView is pretty chaotic with its bounds, as it depends on the scene,
        // the size of the widget, and the scale value to determine it's ultimate value.
        // Because of this, and our start-up process, there are 3-4 different timings of when
        // widgets are added, when they have data, when they display, and when they process events.
        // All this means that this function in particular does not have a consistent point of execution
        // in all of this.
        //
        // A small deviation might be alright, and not immediately noticeable, but since the range this view is
        // operating in is about 200k, a deviation of half a percent is incredibly noticeable.
        //
        // The fix: Listen to all of the events affecting our range(range change, value change for the scrollbars) for a period of time,
        // and constantly re-align ourselves to a fixed left point(also how the graph expands, so this appears fairly natural)
        // in terms of how it handles the window being resized in between loads. Once a reasonable(read magic) amount of time
        // has passed in between resizes, stop doing this, and only listen to events we care about
        // (range change for recalculating our saved anchor point)
        //
        // This oddity handles all of our weird resizing timings.
        m_styleTimer.setInterval(2000);
        m_styleTimer.setSingleShot(true);
        m_styleTimer.start();

        ConnectBoundsSignals();

        // scale back to 1.0
        qreal scaleValue = 1.0 / m_viewParams.m_scale;

        // Apply the new scale
        scaleValue *= viewParams.m_scale;

        // Update view params before setting scale
        m_viewParams = viewParams;

        // Set out scale
        scale(scaleValue, scaleValue);
        OnBoundsChanged();
    }

    void GraphCanvasGraphicsView::DisplayArea(const QRectF& viewArea)
    {
        // This doesn't play well with the weirdness on loading up the ViewParams. So if we are reloading the ViewParams. Just queue this
        // And we can deal with it later.
        if (m_reapplyViewParams)
        {
            if (!m_queuedFocus)
            {
                m_queuedFocus = AZStd::make_unique<FocusQueue>();
            }

            m_queuedFocus->m_focusType = FocusQueue::FocusType::DisplayArea;
            m_queuedFocus->m_focusRect = viewArea;
        }

        {
            // Fit into view.
            fitInView(viewArea, Qt::AspectRatioMode::KeepAspectRatio);

            ClampScaleBounds();

            ViewNotificationBus::Event(GetViewId(), &ViewNotifications::OnViewCenteredOnArea);
        }
    }

    void GraphCanvasGraphicsView::SelectAll()
    {
        SceneRequestBus::Event(m_sceneId, &GraphCanvas::SceneRequests::SelectAll);
    }

    void GraphCanvasGraphicsView::SelectAllRelative(ConnectionType input)
    {
        SceneRequestBus::Event(m_sceneId, &GraphCanvas::SceneRequests::SelectAllRelative, input);
    }

    void GraphCanvasGraphicsView::SelectConnectedNodes()
    {
        SceneRequestBus::Event(m_sceneId, &GraphCanvas::SceneRequests::SelectConnectedNodes);
    }

    void GraphCanvasGraphicsView::ClearSelection()
    {
        SceneRequestBus::Event(m_sceneId, &GraphCanvas::SceneRequests::ClearSelection);
    }

    void GraphCanvasGraphicsView::CenterOnArea(const QRectF& viewArea)
    {
        if (m_reapplyViewParams)
        {
            if (!m_queuedFocus)
            {
                m_queuedFocus = AZStd::make_unique<FocusQueue>();
            }

            m_queuedFocus->m_focusType = FocusQueue::FocusType::CenterOnArea;
            m_queuedFocus->m_focusRect = viewArea;
        }

        {
            qreal originalZoom = transform().m11();

            // Fit into view.
            fitInView(viewArea, Qt::AspectRatioMode::KeepAspectRatio);

            QTransform xfm = transform();

            qreal newZoom = AZ::GetMin(xfm.m11(), originalZoom);

            // Apply the new scale.
            xfm.setMatrix(newZoom, xfm.m12(), xfm.m13(),
                xfm.m21(), newZoom, xfm.m23(),
                xfm.m31(), xfm.m32(), xfm.m33());

            setTransform(xfm);

            ViewNotificationBus::Event(GetViewId(), &ViewNotifications::OnViewCenteredOnArea);

            ClampScaleBounds();
        }
    }

    void GraphCanvasGraphicsView::CenterOn(const QPointF& posInSceneCoordinates)
    {
        centerOn(posInSceneCoordinates);
    }

    void GraphCanvasGraphicsView::CenterOnStartOfChain()
    {
        AZStd::vector< AZ::EntityId > selectedEntities;
        SceneRequestBus::EventResult(selectedEntities, GetScene(), &SceneRequests::GetSelectedNodes);

        AZStd::unordered_set< NodeId > traversedNodes = GraphUtils::FindTerminalForNodeChain(selectedEntities, ConnectionType::CT_Input);

        AZStd::vector< AZ::EntityId > terminalEntities;
        terminalEntities.reserve(traversedNodes.size());
        terminalEntities.assign(traversedNodes.begin(), traversedNodes.end());

        CenterOnSceneMembers(terminalEntities);
    }

    void GraphCanvasGraphicsView::CenterOnEndOfChain()
    {
        AZStd::vector< AZ::EntityId > selectedEntities;
        SceneRequestBus::EventResult(selectedEntities, GetScene(), &SceneRequests::GetSelectedNodes);

        AZStd::unordered_set< NodeId > traversedNodes = GraphUtils::FindTerminalForNodeChain(selectedEntities, ConnectionType::CT_Output);

        AZStd::vector< AZ::EntityId > terminalEntities;
        terminalEntities.reserve(traversedNodes.size());
        terminalEntities.assign(traversedNodes.begin(), traversedNodes.end());

        CenterOnSceneMembers(terminalEntities);
    }

    void GraphCanvasGraphicsView::CenterOnSelection()
    {
        AZStd::vector< AZ::EntityId > selectedEntities;
        SceneRequestBus::EventResult(selectedEntities, GetScene(), &SceneRequests::GetSelectedNodes);

        CenterOnSceneMembers(selectedEntities);
    }

    void GraphCanvasGraphicsView::WheelEvent(QWheelEvent* ev)
    {
        wheelEvent(ev);
    }

    QRectF GraphCanvasGraphicsView::GetViewableAreaInSceneCoordinates()
    {
        return mapToScene(rect()).boundingRect();
    }

    GraphCanvasGraphicsView* GraphCanvasGraphicsView::AsGraphicsView()
    {
        return this;
    }

    QImage* GraphCanvasGraphicsView::CreateImageOfGraph()
    {
        if (m_sceneId.IsValid())
        {
            QRectF sceneArea;
            SceneRequestBus::EventResult(sceneArea, m_sceneId, &SceneRequests::GetSceneBoundingArea);

            return CreateImageOfGraphArea(sceneArea);
        }

        return nullptr;
    }

    QImage* GraphCanvasGraphicsView::CreateImageOfGraphArea(QRectF graphArea)
    {
        if (graphArea.isEmpty())
        {
            graphArea = QRectF(-1, -1, 2, 2);
        }

        QRectF windowSize = graphArea;
        const int maxSize = 17500;
        if (windowSize.width() > maxSize || windowSize.height() > maxSize)
        {
            AzQtComponents::ToastConfiguration toastConfiguration(AzQtComponents::ToastType::Information, "Screenshot", "Screenshot attempted to capture an area too large. Some down-ressing may occur.");
            m_notificationsView->ShowToastNotification(toastConfiguration);

            if (windowSize.width() > maxSize)
            {
                windowSize.setWidth(maxSize);
            }

            if (windowSize.height() > maxSize)
            {
                windowSize.setHeight(maxSize);
            }
        }

        QImage* image = nullptr;
        if (m_sceneId.IsValid())
        {
            graphArea.adjust(-40, -40, 40, 40);
            windowSize.adjust(-40, -40, 40, 40);

            // Ensure that the fixed area is the same
            QGraphicsView* graphicsView = new QGraphicsView();
            graphicsView->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            graphicsView->setFixedWidth(aznumeric_cast<int>(windowSize.width()));
            graphicsView->setFixedHeight(aznumeric_cast<int>(windowSize.height()));
            graphicsView->adjustSize();
            graphicsView->updateGeometry();
            graphicsView->ensurePolished();

            graphicsView->viewport()->adjustSize();
            graphicsView->viewport()->updateGeometry();
            graphicsView->viewport()->ensurePolished();

            QGraphicsScene* graphicsScene = nullptr;
            SceneRequestBus::EventResult(graphicsScene, m_sceneId, &SceneRequests::AsQGraphicsScene);

            graphicsView->setScene(graphicsScene);
            graphicsView->centerOn(graphArea.center());

            qreal xScale = windowSize.width() / graphArea.width();
            qreal yScale = windowSize.height() / graphArea.height();

            if (xScale < yScale)
            {
                graphicsView->scale(xScale, xScale);
            }
            else
            {
                graphicsView->scale(yScale, yScale);
            }

            // Need to display it in the dialog, otherwise the viewport doesn't get updated correctly.
            // and then I can't use it to render out the correct target area.
            //
            // I would move it offscreen to avoid the single frame flash, but I there is no way of doing that that I can see.
            QDialog dialog;

            dialog.setProperty("HasNoWindowDecorations", true);

            QVBoxLayout* layout = new QVBoxLayout();
            layout->addWidget(graphicsView);

            dialog.setLayout(layout);
            dialog.show();
            dialog.hide();

            QRect viewportRect = graphicsView->viewport()->rect();

            image = new QImage(aznumeric_cast<int>(windowSize.width()), aznumeric_cast<int>(windowSize.height()), QImage::Format::Format_ARGB32);
            image->fill(Qt::transparent);

            QPainter localPainter;
            localPainter.begin(image);
            localPainter.setRenderHint(QPainter::Antialiasing);

            graphicsView->render(&localPainter, QRectF(0, 0, windowSize.width(), windowSize.height()), viewportRect);
            localPainter.end();

            AzQtComponents::ToastConfiguration toastConfiguration(
                AzQtComponents::ToastType::Information,
                "<b>Screenshot</b>",
                "Screenshot copied to clipboard!");
            toastConfiguration.m_duration = AZStd::chrono::milliseconds(2000);
            toastConfiguration.m_allowDuplicateNotifications = true;
            m_notificationsView->ShowToastNotification(toastConfiguration);
        }

        return image;
    }

    qreal GraphCanvasGraphicsView::GetZoomLevel() const
    {
        QTransform xfm = transform();

        // Clamp the scaling factor.
        return xfm.m11();
    }

    void GraphCanvasGraphicsView::ScreenshotSelection()
    {
        if (!m_sceneId.IsValid())
        {
            // There's no scene.
            return;
        }

        bool hasSelection = false;
        GraphCanvas::SceneRequestBus::EventResult(hasSelection, m_sceneId, &GraphCanvas::SceneRequests::HasSelectedItems);

        QImage* sceneImage = nullptr;

        if (hasSelection)
        {
            QRectF selectedBoundingRect;
            GraphCanvas::SceneRequestBus::EventResult(selectedBoundingRect, m_sceneId, &GraphCanvas::SceneRequests::GetSelectedSceneBoundingArea);

            if (!selectedBoundingRect.isEmpty())
            {
                sceneImage = CreateImageOfGraphArea(selectedBoundingRect);
            }
            else
            {
                sceneImage = CreateImageOfGraph();
            }
        }
        else
        {
            sceneImage = CreateImageOfGraph();
        }

        if (sceneImage)
        {
            QClipboard* clipboard = QGuiApplication::clipboard();
            clipboard->setImage((*sceneImage));

            delete sceneImage;
        }
    }

    void GraphCanvasGraphicsView::EnableSelection()
    {
        SceneRequestBus::Event(GetScene(), &SceneRequests::EnableSelection);
    }

    void GraphCanvasGraphicsView::DisableSelection()
    {
        SceneRequestBus::Event(GetScene(), &SceneRequests::DisableSelection);
    }

    void GraphCanvasGraphicsView::ShowEntireGraph()
    {
        if (rubberBandRect().isNull() && !QApplication::mouseButtons() && !m_isEditing)
        {
            CenterOnArea(GetCompleteArea());
        }
    }

    void GraphCanvasGraphicsView::ZoomIn()
    {
        if (!m_sceneId.IsValid())
        {
            // There's no scene.
            return;
        }

        QWheelEvent ev(
            QPoint(0, 0),
            mapToGlobal(QPoint(0, 0)),
            QPoint(0, WHEEL_ZOOM * 5),
            QPoint(0, WHEEL_ZOOM_ANGLE * 5),
            Qt::NoButton,
            Qt::NoModifier,
            Qt::NoScrollPhase,
            false
        );
        wheelEvent(&ev);
    }

    void GraphCanvasGraphicsView::ZoomOut()
    {
        if (!m_sceneId.IsValid())
        {
            // There's no scene.
            return;
        }


        QWheelEvent ev(
            QPoint(0, 0),
            mapToGlobal(QPoint(0, 0)),
            QPoint(0, -WHEEL_ZOOM * 5),
            QPoint(0,-WHEEL_ZOOM_ANGLE * 5),
            Qt::NoButton,
            Qt::NoModifier,
            Qt::NoScrollPhase,
            false
        );
        wheelEvent(&ev);
    }

    void GraphCanvasGraphicsView::PanSceneBy(QPointF repositioning, AZStd::chrono::milliseconds duration)
    {
        if (duration.count() == 0)
        {
            centerOn(mapToScene(rect().center()) + repositioning);
        }
        else
        {
            m_panCountdown = duration.count() * 0.001f;

            // Determine how many seconds this needs to take. And convert our speed into pixels/second
            m_panVelocity = repositioning / m_panCountdown;

            ManageTickState();
        }
    }

    void GraphCanvasGraphicsView::PanSceneTo(QPointF scenePoint, AZStd::chrono::milliseconds duration)
    {
        QPointF centerPoint = mapToScene(rect().center());

        PanSceneBy(scenePoint - centerPoint, duration);
    }

    void GraphCanvasGraphicsView::RefreshView()
    {
        invalidateScene(GetViewableAreaInSceneCoordinates());
    }

    void GraphCanvasGraphicsView::HideToastNotification(const AzToolsFramework::ToastId& toastId)
    {
        m_notificationsView->HideToastNotification(toastId);
    }

    AzToolsFramework::ToastId GraphCanvasGraphicsView::ShowToastNotification(const AzQtComponents::ToastConfiguration& toastConfiguration)
    {
        return m_notificationsView->ShowToastNotification(toastConfiguration);
    }

    AzToolsFramework::ToastId GraphCanvasGraphicsView::ShowToastAtCursor(const AzQtComponents::ToastConfiguration& toastConfiguration)
    {
        return m_notificationsView->ShowToastAtCursor(toastConfiguration);
    }

    AzToolsFramework::ToastId GraphCanvasGraphicsView::ShowToastAtPoint(const QPoint& screenPosition, const QPointF& anchorPoint, const AzQtComponents::ToastConfiguration& toastConfiguration)
    {
        return m_notificationsView->ShowToastAtPoint(screenPosition, anchorPoint, toastConfiguration);
    }

    bool GraphCanvasGraphicsView::IsShowing() const
    {
        return isVisible();
    }

    void GraphCanvasGraphicsView::OnTick(float tick, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        if (m_panCountdown)
        {
            float deltaTime = tick;

            if (deltaTime > m_panCountdown)
            {
                deltaTime = m_panCountdown;
                m_panCountdown = 0.0f;

                ManageTickState();
            }
            else
            {
                m_panCountdown -= deltaTime;
            }

            QPointF deltaDistance = m_panVelocity * deltaTime;
            m_panningAggregator += deltaDistance;
        }

        if (!AZ::IsClose(m_edgePanning.first, 0.0f, 0.01f) || !AZ::IsClose(m_edgePanning.second, 0.0f, 0.01f))
        {
            m_panningAggregator += QPointF(m_scrollSpeed * m_edgePanning.first * tick, m_scrollSpeed * m_edgePanning.second * tick);
        }

        centerOn(m_panningAggregator);
    }

    QRectF GraphCanvasGraphicsView::GetCompleteArea()
    {
        // Get the grid.
        AZ::EntityId gridId;
        SceneRequestBus::EventResult(gridId, m_sceneId, &SceneRequests::GetGrid);

        QGraphicsItem* gridItem = nullptr;
        VisualRequestBus::EventResult(gridItem, gridId, &VisualRequests::AsGraphicsItem);

        QGraphicsScene* theScene = scene();
        auto itemList = theScene->items();

        QRectF completeArea = QRectF();

        for (QGraphicsItem* item : itemList)
        {
            if (!item->data(DataIdentifiers::SceneEventFilter).isNull()
                || item == gridItem)
            {
                continue;
            }

            QRectF sceneBoundingRect = item->sceneBoundingRect();

            if (completeArea.isEmpty())
            {
                completeArea = sceneBoundingRect;
            }
            else
            {
                completeArea = completeArea | sceneBoundingRect;
            }
        }

        return completeArea;
    }

    QRectF GraphCanvasGraphicsView::GetSelectedArea()
    {
        QList<QGraphicsItem*> itemsList = items();
        QList<QGraphicsItem*> selectedList;
        QRectF selectedArea;
        QRectF completeArea;
        QRectF area;

        // Get the grid.
        AZ::EntityId gridId;
        SceneRequestBus::EventResult(gridId, m_sceneId, &SceneRequests::GetGrid);

        QGraphicsItem* gridItem = nullptr;
        VisualRequestBus::EventResult(gridItem, gridId, &VisualRequests::AsGraphicsItem);

        // Get the area.
        for (auto item : itemsList)
        {
            if (item == gridItem)
            {
                // Ignore the grid.
                continue;
            }

            completeArea |= item->sceneBoundingRect();

            if (item->isSelected())
            {
                selectedList.push_back(item);
                selectedArea |= item->sceneBoundingRect();
            }
        }

        if (selectedList.empty())
        {
            if (itemsList.size() > 1) // >1 because the grid is always contained.
            {
                // Show everything.
                area = completeArea;
            }
        }
        else
        {
            // Show selection.
            area = selectedArea;
        }

        return area;
    }

    void GraphCanvasGraphicsView::OnStylesChanged()
    {
        update();
    }

    void GraphCanvasGraphicsView::OnNodeIsBeingEdited(bool isEditing)
    {
        m_isEditing = isEditing;
    }

    void GraphCanvasGraphicsView::keyReleaseEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_Control:
            if (rubberBandRect().isNull() && !QApplication::mouseButtons())
            {
                setDragMode(QGraphicsView::RubberBandDrag);
                setInteractive(true);
            }
            break;
        case Qt::Key_Alt:
        {
            static const bool enabled = false;
            ViewSceneNotificationBus::Event(GetScene(), &ViewSceneNotifications::OnAltModifier, enabled);
            break;
        }
        case Qt::Key_Escape:
        {
            ViewNotificationBus::Event(m_viewId, &ViewNotifications::OnEscape);
            break;
        }
        default:
            break;
        }

        QGraphicsView::keyReleaseEvent(event);
    }

    void GraphCanvasGraphicsView::keyPressEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_Alt:
        {
            static const bool enabled = true;
            ViewSceneNotificationBus::Event(GetScene(), &ViewSceneNotifications::OnAltModifier, enabled);
            break;
        }
        default:
            break;
        }

        QGraphicsView::keyPressEvent(event);
    }

    void GraphCanvasGraphicsView::contextMenuEvent(QContextMenuEvent *event)
    {
        if (event->reason() == QContextMenuEvent::Mouse)
        {
            // invoke the context menu in mouseReleaseEvent, otherwise we cannot drag with right mouse button
            return;
        }
        QGraphicsView::contextMenuEvent(event);
    }

    void GraphCanvasGraphicsView::mousePressEvent(QMouseEvent* event)
    {
        // If we already have a mouse button down, we just want to ignore it.
        if (!event->buttons().testFlag(event->button()))
        {
            // Even if we don't handle the mouse event here, or pass it down. The context menu still occurs.
            // Just suppress the next context menu since we know it will occur when this 'ignored'
            // mouse button releases
            if (!m_checkForDrag && (event->button() == Qt::RightButton))
            {
                SceneRequestBus::Event(m_sceneId, &SceneRequests::SuppressNextContextMenu);
            }

            return;
        }

        if ((event->button() == Qt::RightButton || event->button() == Qt::MiddleButton))
        {
            m_initialClick = event->pos();
            m_checkForDrag = true;
            event->accept();
            return;
        }
        else if (event->button() == Qt::LeftButton)
        {
            m_checkForEdges = true;
        }

        QGraphicsView::mousePressEvent(event);
    }

    void GraphCanvasGraphicsView::mouseMoveEvent(QMouseEvent* event)
    {
        if ((event->buttons() & (Qt::RightButton | Qt::MiddleButton)) == Qt::NoButton)
        {
            // it might be that the mouse button already was released but the mouseReleaseEvent
            // was sent somewhere else (context menu)
            m_checkForDrag = false;
        }
        else if (m_checkForDrag && isInteractive())
        {
            event->accept();

            // If we move ~0.5% in both directions, we'll consider it an intended move
            if ((m_initialClick - event->pos()).manhattanLength() > (width() * 0.005f + height() * 0.005f))
            {
                setDragMode(QGraphicsView::ScrollHandDrag);
                setInteractive(false);
                // This is done because the QGraphicsView::mousePressEvent implementation takes care of
                // scrolling only when the DragMode is ScrollHandDrag is set and the mouse LeftButton is pressed
                /* QT comment/
                // Left-button press in scroll hand mode initiates hand scrolling.
                */
                QMouseEvent startPressEvent(QEvent::MouseButtonPress, m_initialClick, Qt::LeftButton, Qt::LeftButton, event->modifiers());
                QGraphicsView::mousePressEvent(&startPressEvent);

                QMouseEvent customEvent(event->type(), event->pos(), Qt::LeftButton, Qt::LeftButton, event->modifiers());
                QGraphicsView::mousePressEvent(&customEvent);
            }

            return;
        }

        if (m_checkForEdges)
        {
            m_edgePanning = CalculateEdgePanning(event->globalPos());
            ManageTickState();
        }

        QGraphicsView::mouseMoveEvent(event);
    }

    void GraphCanvasGraphicsView::mouseReleaseEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::RightButton)
        {
            // If we move less than ~0.5% in both directions, we'll consider it an unintended move
            if ((m_initialClick - event->pos()).manhattanLength() <= (width() * 0.005f + height() * 0.005f))
            {
                QContextMenuEvent ce(QContextMenuEvent::Mouse, event->pos(), event->globalPos(), event->modifiers());
                QGraphicsView::contextMenuEvent(&ce);
                return;
            }
        }
        if (event->button() == Qt::RightButton || event->button() == Qt::MiddleButton)
        {
            m_checkForDrag = false;

            if (!isInteractive())
            {
                // This is done because the QGraphicsView::mouseReleaseEvent implementation takes care of
                // setting the mouse cursor back to default
                /* QT Comment
                // Restore the open hand cursor. ### There might be items
                // under the mouse that have a valid cursor at this time, so
                // we could repeat the steps from mouseMoveEvent().
                */
                QMouseEvent customEvent(event->type(), event->pos(), Qt::LeftButton, Qt::LeftButton, event->modifiers());
                QGraphicsView::mouseReleaseEvent(&customEvent);
                event->accept();
                setInteractive(true);
                setDragMode(QGraphicsView::RubberBandDrag);

                SaveViewParams();
                return;
            }
        }

        if (event->button() == Qt::LeftButton)
        {
            m_checkForEdges = false;
            m_edgePanning = AZStd::pair<float, float>(0.0f, 0.0f);
            ManageTickState();
        }

        QGraphicsView::mouseReleaseEvent(event);
    }

    void GraphCanvasGraphicsView::wheelEvent(QWheelEvent* event)
    {
        if (!(event->modifiers() & Qt::ControlModifier))
        {
            setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
            // Scale the view / do the zoom
            // delta is 1/8th of a degree. We want to change the zoom in or out 0.01 per degree.
            // so: 1/8/100 = 1/800 = 0.00125
            qreal scaleFactor = 1.0 + (event->angleDelta().y() * 0.00125);
            qreal newScale = m_viewParams.m_scale * scaleFactor;

            if (newScale < m_minZoom)
            {
                newScale = m_minZoom;
                scaleFactor = (m_minZoom / m_viewParams.m_scale);
            }
            else if (newScale > m_maxZoom)
            {
                newScale = m_maxZoom;
                scaleFactor = (m_maxZoom / m_viewParams.m_scale);
            }

            m_viewParams.m_scale = newScale;
            scale(scaleFactor, scaleFactor);

            QueueSave();
            event->accept();
            setTransformationAnchor(QGraphicsView::AnchorViewCenter);

            ViewNotificationBus::Event(GetViewId(), &ViewNotifications::OnZoomChanged, m_viewParams.m_scale);
        }
    }

    void GraphCanvasGraphicsView::focusOutEvent(QFocusEvent* event)
    {
        QGraphicsView::focusOutEvent(event);

        ViewNotificationBus::Event(GetViewId(), &ViewNotifications::OnFocusLost);
    }

    void GraphCanvasGraphicsView::resizeEvent(QResizeEvent* event)
    {
        ViewNotificationBus::Event(GetViewId(), &ViewNotifications::OnViewResized, event);

        QGraphicsView::resizeEvent(event);

        if (scene())
        {
            CalculateMinZoomBounds();
            ClampScaleBounds();
        }

        CalculateInternalRectangle();

        m_notificationsView->UpdateToastPosition();
    }

    void GraphCanvasGraphicsView::moveEvent(QMoveEvent* event)
    {
        QGraphicsView::moveEvent(event);

        m_notificationsView->UpdateToastPosition();
    }

    void GraphCanvasGraphicsView::scrollContentsBy(int dx, int dy)
    {
        ViewNotificationBus::Event(GetViewId(), &ViewNotifications::OnViewScrolled);

        QGraphicsView::scrollContentsBy(dx, dy);
    }

    void GraphCanvasGraphicsView::showEvent(QShowEvent* showEvent)
    {
        QGraphicsView::showEvent(showEvent);

        m_notificationsView->OnShow();
    }

    void GraphCanvasGraphicsView::hideEvent(QHideEvent* hideEvent)
    {
        QGraphicsView::hideEvent(hideEvent);

        m_notificationsView->OnHide();
    }

    void GraphCanvasGraphicsView::OnSettingsChanged()
    {
        auto settingsHandler = AssetEditorSettingsRequestBus::FindFirstHandler(GetEditorId());

        if (settingsHandler)
        {
            m_scrollSpeed = settingsHandler->GetEdgePanningScrollSpeed();
            m_maxZoom = settingsHandler->GetMaxZoom();

            ClampScaleBounds();
            CalculateInternalRectangle();
        }
    }

    void GraphCanvasGraphicsView::OnEscape()
    {
        ClearSelection();
    }

    void GraphCanvasGraphicsView::CreateBookmark(int bookmarkShortcut)
    {
        AZ::EntityId sceneId = GetScene();

        bool remapId = false;
        AZ::EntityId bookmark;

        AZStd::vector< AZ::EntityId > selectedItems;
        SceneRequestBus::EventResult(selectedItems, sceneId, &SceneRequests::GetSelectedItems);

        for (const AZ::EntityId& selectedItem : selectedItems)
        {
            if (BookmarkRequestBus::FindFirstHandler(selectedItem) != nullptr)
            {
                if (bookmark.IsValid())
                {
                    remapId = false;
                }
                else
                {
                    remapId = true;
                    bookmark = selectedItem;
                }
            }
        }

        AZ::EntityId existingBookmark;
        BookmarkManagerRequestBus::EventResult(existingBookmark, sceneId, &BookmarkManagerRequests::FindBookmarkForShortcut, bookmarkShortcut);

        if (existingBookmark.IsValid() && (!remapId || bookmark != existingBookmark))
        {
            AZStd::string bookmarkName;
            BookmarkRequestBus::EventResult(bookmarkName, existingBookmark, &BookmarkRequests::GetBookmarkName);

            QMessageBox::StandardButton response = QMessageBox::StandardButton::No;
            response = QMessageBox::question(this, QString("Bookmarking Conflict"), QString("Bookmark (%1) already registered with shortcut (%2).\nProceed with action and remove previous bookmark?").arg(bookmarkName.c_str()).arg(bookmarkShortcut), QMessageBox::StandardButton::Yes|QMessageBox::No);

            if (response == QMessageBox::StandardButton::No)
            {
                return;
            }
            else if (response == QMessageBox::StandardButton::Yes)
            {
                BookmarkRequestBus::Event(existingBookmark, &BookmarkRequests::RemoveBookmark);
            }
        }

        if (remapId)
        {
            BookmarkManagerRequestBus::Event(sceneId, &BookmarkManagerRequests::RequestShortcut, bookmark, bookmarkShortcut);
            GraphModelRequestBus::Event(sceneId, &GraphModelRequests::RequestUndoPoint);
        }
        else
        {
            bool createdAnchor = false;
            AZ::Vector2 position = GetViewSceneCenter();
            BookmarkManagerRequestBus::EventResult(createdAnchor, sceneId, &BookmarkManagerRequests::CreateBookmarkAnchor, position, bookmarkShortcut);

            if (createdAnchor)
            {
                GraphModelRequestBus::Event(sceneId, &GraphModelRequests::RequestUndoPoint);
            }
        }
    }


    void GraphCanvasGraphicsView::JumpToBookmark(int bookmarkShortcut)
    {
        AZ::EntityId sceneId = GetScene();
        BookmarkManagerRequestBus::Event(sceneId, &BookmarkManagerRequests::ActivateShortcut, bookmarkShortcut);
    }

    void GraphCanvasGraphicsView::CenterOnSceneMembers(const AZStd::vector<AZ::EntityId>& memberIds)
    {
        QRectF boundingRect;

        for (auto memberId : memberIds)
        {
            QGraphicsItem* graphicsItem = nullptr;
            SceneMemberUIRequestBus::EventResult(graphicsItem, memberId, &SceneMemberUIRequests::GetRootGraphicsItem);

            if (graphicsItem)
            {
                boundingRect |= graphicsItem->sceneBoundingRect();
            }
        }

        if (!boundingRect.isEmpty())
        {
            CenterOnArea(boundingRect);
        }
    }

    void GraphCanvasGraphicsView::ConnectBoundsSignals()
    {
        m_reapplyViewParams = true;
        connect(horizontalScrollBar(), &QScrollBar::rangeChanged, this, &GraphCanvasGraphicsView::OnBoundsChanged);
        connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &GraphCanvasGraphicsView::OnBoundsChanged);
    }

    void GraphCanvasGraphicsView::DisconnectBoundsSignals()
    {
        if (m_reapplyViewParams)
        {
            m_reapplyViewParams = false;
            disconnect(horizontalScrollBar(), &QScrollBar::rangeChanged, this, &GraphCanvasGraphicsView::OnBoundsChanged);
            disconnect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &GraphCanvasGraphicsView::OnBoundsChanged);

            if (m_queuedFocus)
            {
                m_queuedFocus.reset();
            }
        }
    }

    void GraphCanvasGraphicsView::OnBoundsChanged()
    {
        if (m_reapplyViewParams)
        {
            if (!m_ignoreValueChange && isInteractive())
            {
                QScopedValueRollback<bool> scopedSetter(m_ignoreValueChange, true);
                m_styleTimer.stop();
                m_styleTimer.start();

                QPointF knownAnchor = mapToScene(rect().topLeft());
                QPointF desiredPoint(m_viewParams.m_anchorPointX, m_viewParams.m_anchorPointY);
                QPointF displacementVector = desiredPoint - knownAnchor;

                QPointF centerPoint = mapToScene(rect().center());
                centerPoint += displacementVector;

                centerOn(centerPoint);

                if (m_queuedFocus)
                {
                    // Work around for not being able to pass a force param into the EBus event.
                    // Just disable the flag we're using to gate it and re-apply it.
                    if (m_queuedFocus->m_focusType == FocusQueue::FocusType::CenterOnArea)
                    {
                        CenterOnArea(m_queuedFocus->m_focusRect);
                    }
                    else
                    {
                        DisplayArea(m_queuedFocus->m_focusRect);
                    }
                }
            }
        }
        else
        {
            QueueSave();
        }
    }

    void GraphCanvasGraphicsView::QueueSave()
    {
        m_timer.stop();
        m_timer.start(250);
    }

    void GraphCanvasGraphicsView::SaveViewParams()
    {
        QPointF anchorPoint = mapToScene(rect().topLeft());

        m_viewParams.m_anchorPointX = aznumeric_cast<float>(anchorPoint.x());
        m_viewParams.m_anchorPointY = aznumeric_cast<float>(anchorPoint.y());

        ViewNotificationBus::Event(GetViewId(), &ViewNotifications::OnViewParamsChanged, m_viewParams);
    }

    void GraphCanvasGraphicsView::CalculateMinZoomBounds()
    {
        if (scene())
        {
            QRectF sceneRect = scene()->sceneRect();

            qreal horizontalScale = static_cast<qreal>(width()) / static_cast<qreal>(sceneRect.width());
            qreal verticalScale = static_cast<qreal>(height()) / static_cast<qreal>(sceneRect.height());

            m_minZoom = AZ::GetMax(horizontalScale, verticalScale);
        }
    }

    void GraphCanvasGraphicsView::ClampScaleBounds()
    {
        QTransform xfm = transform();

        // Clamp the scaling factor.
        if (m_minZoom < m_maxZoom)
        {
            m_viewParams.m_scale = AZ::GetClamp(xfm.m11(), m_minZoom, m_maxZoom);
        }
        else if (m_minZoom < m_maxZoom)
        {
            m_viewParams.m_scale = AZ::GetClamp(xfm.m11(), m_maxZoom, m_minZoom);
        }

        // Apply the new scale
        xfm.setMatrix(m_viewParams.m_scale, xfm.m12(), xfm.m13(),
            xfm.m21(), m_viewParams.m_scale, xfm.m23(),
            xfm.m31(), xfm.m32(), xfm.m33());
        setTransform(xfm);

        ViewNotificationBus::Event(GetViewId(), &ViewNotifications::OnZoomChanged, m_viewParams.m_scale);
    }

    void GraphCanvasGraphicsView::OnRubberBandChanged([[maybe_unused]] QRect rubberBandRect, QPointF fromScenePoint, QPointF toScenePoint)
    {
        if (fromScenePoint.isNull() && toScenePoint.isNull())
        {
            if (m_isDragSelecting)
            {
                m_isDragSelecting = false;
                SceneRequestBus::Event(m_sceneId, &SceneRequests::SignalDragSelectEnd);
            }
        }
        else if (!m_isDragSelecting)
        {
            m_isDragSelecting = true;
            SceneRequestBus::Event(m_sceneId, &SceneRequests::SignalDragSelectStart);
        }
    }

    void GraphCanvasGraphicsView::CalculateInternalRectangle()
    {
        float edgePercentage = 0.0f;
        AssetEditorSettingsRequestBus::EventResult(edgePercentage, GetEditorId(), &AssetEditorSettingsRequests::GetEdgePanningPercentage);

        float widthOffset = size().width() * edgePercentage;
        float heightOffset = size().height() * edgePercentage;

        m_offsets.setX(widthOffset);
        m_offsets.setY(heightOffset);

        m_internalRectangle = rect();
        m_internalRectangle.adjust(widthOffset, heightOffset, -widthOffset, -heightOffset);
    }

    AZStd::pair<float, float> GraphCanvasGraphicsView::CalculateEdgePanning(const QPointF& globalPoint) const
    {
        QPointF screenPoint = mapFromGlobal(globalPoint.toPoint());

        AZStd::pair<float, float> edgeConfig = AZStd::pair<float, float>(0.0f, 0.0f);

        float horizontalDifference = 0.0f;

        if (screenPoint.x() < m_internalRectangle.left())
        {
            horizontalDifference = aznumeric_cast<float>(screenPoint.x() - m_internalRectangle.left());
        }
        else if (screenPoint.x() > m_internalRectangle.right())
        {
            horizontalDifference = aznumeric_cast<float>(screenPoint.x() - m_internalRectangle.right());
        }

        float verticalDifference = 0.0f;

        if (screenPoint.y() < m_internalRectangle.top())
        {
            verticalDifference = aznumeric_cast<float>(screenPoint.y() - m_internalRectangle.top());
        }
        else if (screenPoint.y() > m_internalRectangle.bottom())
        {
            verticalDifference = aznumeric_cast<float>(screenPoint.y() - m_internalRectangle.bottom());
        }

        if (AZ::IsClose(m_offsets.x(), 0.0, 0.001))
        {
            edgeConfig.first = 0.0f;
        }
        else
        {
            edgeConfig.first = 10.0f * AZStd::clamp(static_cast<float>(horizontalDifference / m_offsets.x()), -1.0f, 1.0f);
        }

        if (AZ::IsClose(m_offsets.y(), 0.0, 0.001))
        {
            edgeConfig.second = 0.0f;
        }
        else
        {
            edgeConfig.second = 10.0f * AZStd::clamp(static_cast<float>(verticalDifference / m_offsets.y()), -1.0f, 1.0f);
        }

        qreal zoomLevel = GetZoomLevel();

        if (AZ::IsClose(zoomLevel, qreal(0.0f), qreal(0.001f)))
        {
            zoomLevel = qreal(1.0f);
        }

        float zoomRepresentation = aznumeric_cast<float>(1.0f / zoomLevel);
        float modifier = AZStd::max(0.5f, zoomRepresentation);

        edgeConfig.first *= modifier;
        edgeConfig.second *= modifier;

        return edgeConfig;
    }

    void GraphCanvasGraphicsView::ManageTickState()
    {
        if ((!AZ::IsClose(m_edgePanning.first, 0.0f, 0.01f) || !AZ::IsClose(m_edgePanning.second, 0.0f, 0.01f))
            || m_panCountdown > 0.0f)
        {
            if (!AZ::TickBus::Handler::BusIsConnected())
            {
                m_panningAggregator = mapToScene(rect().center());
                AZ::TickBus::Handler::BusConnect();
            }
        }
        else if (AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
    }
}
