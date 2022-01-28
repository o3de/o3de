/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CanvasWidget.h"

AZ_PUSH_DISABLE_WARNING(4244 4251 4800, "-Wunknown-warning-option")
#include <QVBoxLayout>
#include <QLabel>
#include <QGraphicsView>
#include <QPushButton>

#include "Editor/View/Widgets/ui_CanvasWidget.h"
AZ_POP_DISABLE_WARNING

#include <AzCore/Casting/numeric_cast.h>



#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <GraphCanvas/Widgets/MiniMapGraphicsView/MiniMapGraphicsView.h>
#include <GraphCanvas/GraphCanvasBus.h>

#include <Debugger/Bus.h>
#include <Core/Graph.h>
#include <Editor/View/Dialogs/SettingsDialog.h>
#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        CanvasWidget::CanvasWidget(const ScriptCanvasEditor::SourceHandle& assetId, QWidget* parent)
            : QWidget(parent)
            , ui(new Ui::CanvasWidget())
            , m_attached(false)
            , m_assetId(assetId)
            , m_graphicsView(nullptr)
            , m_miniMapView(nullptr)
        {
            ui->setupUi(this);
            ui->m_debuggingControls->hide();

            SetupGraphicsView();

            connect(ui->m_debugAttach, &QPushButton::clicked, this, &CanvasWidget::OnClicked);
        }

        CanvasWidget::~CanvasWidget()
        {
            hide();
        }

        void CanvasWidget::SetDefaultBorderColor(AZ::Color defaultBorderColor)
        {
            m_defaultBorderColor = defaultBorderColor;

            QString styleSheet = QString("QFrame#graphicsViewFrame { background-color: rgb(%1,%2,%3) }").arg(defaultBorderColor.GetR8()).arg(defaultBorderColor.GetG8()).arg(defaultBorderColor.GetB8());

            ui->graphicsViewFrame->setStyleSheet(styleSheet);
        }

        void CanvasWidget::ShowScene(const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
        {
            EditorGraphRequests* editorGraphRequests = EditorGraphRequestBus::FindFirstHandler(scriptCanvasId);
            editorGraphRequests->CreateGraphCanvasScene();
            AZ::EntityId graphCanvasSceneId = editorGraphRequests->GetGraphCanvasGraphId();
            m_graphicsView->SetScene(graphCanvasSceneId);
            m_scriptCanvasId = scriptCanvasId;
        }

        void CanvasWidget::SetAssetId(const ScriptCanvasEditor::SourceHandle& assetId)
        {
            m_assetId = assetId;
        }

        const GraphCanvas::ViewId& CanvasWidget::GetViewId() const
        {
            return m_graphicsView->GetViewId();
        }

        void CanvasWidget::EnableView()
        {
            if (!isEnabled())
            {
                setDisabled(false);

                if (m_disabledOverlay.IsValid() && m_graphicsView)
                {
                    GraphCanvas::SceneRequestBus::Event(m_graphicsView->GetScene(), &GraphCanvas::SceneRequests::CancelGraphicsEffect, m_disabledOverlay);
                    m_disabledOverlay = GraphCanvas::GraphicsEffectId();
                }
            }
        }

        void CanvasWidget::DisableView()
        {
            if (isEnabled())
            {
                setDisabled(true);

                if (m_graphicsView)
                {
                    AZ::EntityId graphCanvasSceneId = m_graphicsView->GetScene();

                    GraphCanvas::OccluderConfiguration occluderConfiguration;

                    occluderConfiguration.m_bounds = m_graphicsView->sceneRect();
                    occluderConfiguration.m_opacity = 0.5f;
                    occluderConfiguration.m_renderColor = QColor(0, 0, 0);
                    occluderConfiguration.m_zValue = 100000;

                    GraphCanvas::SceneRequestBus::EventResult(m_disabledOverlay, graphCanvasSceneId, &GraphCanvas::SceneRequests::CreateOccluder, occluderConfiguration);
                }
            }
        }

        void CanvasWidget::SetupGraphicsView()
        {
            const bool registerMenuActions = false;
            m_graphicsView = aznew GraphCanvas::GraphCanvasGraphicsView(nullptr, registerMenuActions);

            AZ_Assert(m_graphicsView, "Could Canvas Widget unable to create CanvasGraphicsView object.");
            if (m_graphicsView)
            {
                ui->graphicsViewFrame->layout()->addWidget(m_graphicsView);

                m_graphicsView->show();                
                m_graphicsView->SetEditorId(ScriptCanvasEditor::AssetEditorId);

                // Temporary shortcut for docking the MiniMap. Removed until we fix up the MiniMap
                /*
                {
                    QAction* action = new QAction(m_graphicsView);
                    action->setShortcut(QKeySequence(Qt::Key_M));
                    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);

                    connect(action, &QAction::triggered,
                        [this]()
                        {
                            if (!m_graphicsView->rubberBandRect().isNull() || QApplication::mouseButtons() || m_graphicsView->GetIsEditing())
                            {
                                // Nothing to do.
                                return;
                            }

                            if (m_miniMapView)
                            {
                                // Cycle the position.
                                m_miniMapPosition = static_cast<MiniMapPosition>((m_miniMapPosition + 1) % MM_Position_Count);
                            }
                            else
                            {
                                m_miniMapView = aznew GraphCanvas::MiniMapGraphicsView(0 , false, m_graphicsView->GetScene(), m_graphicsView);
                            }

                            // Apply position.
                            PositionMiniMap();
                        });

                    m_graphicsView->addAction(action);
                }*/
            }
        }

        void CanvasWidget::showEvent(QShowEvent* /*event*/)
        {
            ui->m_debugAttach->setText(m_attached ? "Debugging: On" : "Debugging: Off");

            EditorGraphRequestBus::Event(m_scriptCanvasId, &EditorGraphRequests::OnGraphCanvasSceneVisible);
        }

        void CanvasWidget::PositionMiniMap()
        {
            if (!(m_miniMapView && m_graphicsView))
            {
                // Nothing to do.
                return;
            }

            const QRect& parentRect = m_graphicsView->frameGeometry();

            if (m_miniMapPosition == MM_Upper_Left)
            {
                m_miniMapView->move(0, 0);
            }
            else if (m_miniMapPosition == MM_Upper_Right)
            {
                m_miniMapView->move(parentRect.width() - m_miniMapView->size().width(), 0);
            }
            else if (m_miniMapPosition == MM_Lower_Right)
            {
                m_miniMapView->move(parentRect.width() - m_miniMapView->size().width(), parentRect.height() - m_miniMapView->size().height());
            }
            else if (m_miniMapPosition == MM_Lower_Left)
            {
                m_miniMapView->move(0, parentRect.height() - m_miniMapView->size().height());
            }

            m_miniMapView->setVisible(m_miniMapPosition != MM_Not_Visible);
        }

        void CanvasWidget::resizeEvent(QResizeEvent *ev)
        {
            QWidget::resizeEvent(ev);

            PositionMiniMap();
        }

        void CanvasWidget::OnClicked()
        {
        }

#include <Editor/View/Widgets/moc_CanvasWidget.cpp>

    }
}
