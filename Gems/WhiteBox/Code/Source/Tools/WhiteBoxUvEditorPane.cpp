/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Tools/WhiteBoxUvEditorPane.h"
#include "EditorWhiteBoxComponent.h"
#include "SubComponentModes/EditorWhiteBoxTransformModeBus.h"
#include "Util/WhiteBoxEditorUtil.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

#include <QAction>
#include <QActionGroup>
#include <QIcon>
#include <QToolBar>
#include <QEvent>
#include <QGraphicsEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

namespace WhiteBox
{
    namespace
    {
        // The first selected entity carrying a White Box, or an invalid pair.
        AZ::EntityComponentIdPair SelectedWhiteBox()
        {
            AzToolsFramework::EntityIdList selected;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                selected, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
            for (const AZ::EntityId entityId : selected)
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
                if (auto* whiteBox = entity != nullptr ? entity->FindComponent<EditorWhiteBoxComponent>() : nullptr)
                {
                    return AZ::EntityComponentIdPair(entityId, whiteBox->GetId());
                }
            }
            return AZ::EntityComponentIdPair();
        }
    } // namespace

    WhiteBoxUvEditorPane::WhiteBoxUvEditorPane(QWidget* parent)
        : QWidget(parent)
    {
        // Made first: the toolbar's actions talk to it, and checking the default tool fires one straight away.
        m_canvas = new WhiteBoxUvCanvas(this);
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // One icon toolbar: transform tools, reshaping, seams, layout, selection, then Frame on the far side.
        auto* toolbar = new QToolBar(this);
        toolbar->setIconSize(QSize(20, 20));
        toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        toolbar->setMovable(false);
        layout->addWidget(toolbar);
        const auto addAction = [toolbar](const QString& icon, const QString& name, const QString& tip)
        {
            QAction* action = toolbar->addAction(QIcon(icon), name);
            action->setToolTip(QStringLiteral("<b>%1</b><br>%2").arg(name, tip));
            return action;
        };
        const QString editorIcons = QStringLiteral(":/stylesheet/img/UI20/toolbar/");
        const QString whiteBoxIcons = QStringLiteral(":/WhiteBox/Icons/");

        auto* tools = new QActionGroup(this);
        tools->setExclusive(true);
        const auto addTool = [&](const QString& icon, const QString& name, const QString& tip, const WhiteBoxUvCanvas::Tool tool)
        {
            QAction* action = addAction(editorIcons + icon, name, tip);
            action->setCheckable(true);
            tools->addAction(action);
            connect(action, &QAction::toggled, this, [this, tool](bool on) { if (on) { m_canvas->SetTool(tool); } });
            return action;
        };
        addTool(QStringLiteral("Move.svg"), tr("Move"), tr("Drag selected UV points to move them. Ctrl snaps to 1/32."), WhiteBoxUvCanvas::Tool::Move)
            ->setChecked(true);
        addTool(QStringLiteral("Rotate.svg"), tr("Rotate"), tr("Drag to rotate the selection about its centre. Ctrl snaps to 15 degrees."),
            WhiteBoxUvCanvas::Tool::Rotate);
        addTool(QStringLiteral("Scale.svg"), tr("Scale"), tr("Drag to scale the selection about its centre. Ctrl snaps to tenths."),
            WhiteBoxUvCanvas::Tool::Scale);
        toolbar->addSeparator();

        connect(addAction(whiteBoxIcons + QStringLiteral("UvFlipU.svg"), tr("Flip U"), tr("Mirror the selection (or everything) left to right.")),
            &QAction::triggered, this, [this]()
            {
                m_canvas->TransformTargets([](const AZ::Vector2& uv, const AZ::Vector2& centre)
                {
                    return AZ::Vector2(2.0f * centre.GetX() - uv.GetX(), uv.GetY());
                });
            });
        connect(addAction(whiteBoxIcons + QStringLiteral("UvFlipV.svg"), tr("Flip V"), tr("Mirror the selection (or everything) top to bottom.")),
            &QAction::triggered, this, [this]()
            {
                m_canvas->TransformTargets([](const AZ::Vector2& uv, const AZ::Vector2& centre)
                {
                    return AZ::Vector2(uv.GetX(), 2.0f * centre.GetY() - uv.GetY());
                });
            });
        connect(addAction(whiteBoxIcons + QStringLiteral("UvRotate90.svg"), tr("Rotate 90"), tr("Turn the selection (or everything) a quarter turn clockwise.")),
            &QAction::triggered, this, [this]()
            {
                // V runs down the view, so (x, y) -> (-y, x) turns clockwise on screen.
                m_canvas->TransformTargets([](const AZ::Vector2& uv, const AZ::Vector2& centre)
                {
                    const AZ::Vector2 local = uv - centre;
                    return centre + AZ::Vector2(-local.GetY(), local.GetX());
                });
            });
        connect(addAction(whiteBoxIcons + QStringLiteral("UvFit.svg"), tr("Fit 0-1"), tr("Scale and move the selection (or everything) uniformly to fill the unit square.")),
            &QAction::triggered, this, [this]() { m_canvas->FitTargetsToUnitSquare(); });
        toolbar->addSeparator();

        connect(addAction(whiteBoxIcons + QStringLiteral("UvSplit.svg"), tr("Split"), tr("Tear the selected faces' UVs away from their neighbours so they move on their own.")),
            &QAction::triggered, this, [this]()
            {
                const size_t faces = m_canvas->SplitSelectedFaces();
                m_message->setText(
                    faces == 0 ? tr("Select whole faces (every corner) to split them off.")
                               : tr("%1 face(s) split off; drag them away to open the seam.").arg(faces));
                Refresh();
            });
        connect(addAction(whiteBoxIcons + QStringLiteral("UvSew.svg"), tr("Sew"), tr("Join selected UV points that belong to the same mesh vertex, at their average.")),
            &QAction::triggered, this, [this]()
            {
                const size_t points = m_canvas->SewSelected();
                m_message->setText(
                    points == 0 ? tr("Select UV points on both sides of a seam; points of the same mesh vertex are joined.")
                                : tr("%1 point(s) sewn.").arg(points));
            });
        toolbar->addSeparator();

        connect(addAction(whiteBoxIcons + QStringLiteral("UvUnwrap.svg"), tr("Unwrap"),
                    tr("Unfold the selected faces (or all in view) flat, each polygon at its true shape, hinged on shared edges; "
                       "overlaps start new islands. The result is packed into the unit square.")),
            &QAction::triggered, this, [this]()
            {
                auto* component = FindWhiteBoxComponent(m_pair);
                if (const WhiteBoxMesh* mesh = component != nullptr ? component->GetWhiteBoxMesh() : nullptr)
                {
                    ApplyLayout(UvOps::Unwrap(*mesh, m_canvas->TargetFaces()), tr("Unwrapped and packed."));
                }
            });
        connect(addAction(whiteBoxIcons + QStringLiteral("UvPack.svg"), tr("Pack"), tr("Arrange the islands of the selected faces (or all in view) in the unit square without overlap.")),
            &QAction::triggered, this, [this]()
            {
                auto* component = FindWhiteBoxComponent(m_pair);
                if (const WhiteBoxMesh* mesh = component != nullptr ? component->GetWhiteBoxMesh() : nullptr)
                {
                    ApplyLayout(UvOps::Pack(*mesh, m_canvas->TargetFaces()), tr("Islands packed."));
                }
            });
        toolbar->addSeparator();

        // Selection works in UV space, so seams bound it.
        connect(addAction(whiteBoxIcons + QStringLiteral("UvSelectAll.svg"), tr("Select All"), tr("Select every UV point. [A]")),
            &QAction::triggered, this, [this]() { m_canvas->SelectAll(); });
        connect(addAction(whiteBoxIcons + QStringLiteral("SelectLinked.svg"), tr("Select Linked"), tr("Every island holding a selected point. [L, or double-click a point]")),
            &QAction::triggered, this, [this]() { m_canvas->SelectLinked(); });
        connect(addAction(whiteBoxIcons + QStringLiteral("GrowSelection.svg"), tr("Grow Selection"), tr("Add every point one UV edge away from the selection. [+]")),
            &QAction::triggered, this, [this]() { m_canvas->GrowSelection(); });
        connect(addAction(whiteBoxIcons + QStringLiteral("ShrinkSelection.svg"), tr("Shrink Selection"), tr("Drop every selected point that touches an unselected one. [-]")),
            &QAction::triggered, this, [this]() { m_canvas->ShrinkSelection(); });
        connect(addAction(whiteBoxIcons + QStringLiteral("EdgeLoop.svg"), tr("Select Loop"),
                    tr("From each selected edge, follow polygon edges straight on through each point until the line turns or ends.")),
            &QAction::triggered, this, [this]()
            {
                if (!m_canvas->SelectLoop())
                {
                    m_message->setText(tr("Select the two ends of a polygon edge to follow its loop."));
                }
            });
        connect(addAction(whiteBoxIcons + QStringLiteral("UvSelectBorder.svg"), tr("Select Border"), tr("Points on island boundaries; limited to islands with a selection, if any.")),
            &QAction::triggered, this, [this]() { m_canvas->SelectBorder(); });
        connect(addAction(whiteBoxIcons + QStringLiteral("UvSelectInvert.svg"), tr("Invert Selection"), tr("Swap selected and unselected points. [Ctrl+I]")),
            &QAction::triggered, this, [this]() { m_canvas->InvertSelection(); });
        connect(addAction(whiteBoxIcons + QStringLiteral("UvSelectNone.svg"), tr("Select None"), tr("Clear the selection. [Esc]")),
            &QAction::triggered, this, [this]() { m_canvas->SelectNone(); });

        auto* spacer = new QWidget(toolbar);
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        toolbar->addWidget(spacer);
        m_showTexture = addAction(editorIcons + QStringLiteral("Material.svg"), tr("Show Texture"),
            tr("Draw the faces' base colour texture behind the UVs, repeating outside the unit square as it does on the mesh."));
        m_showTexture->setCheckable(true);
        m_showTexture->setChecked(true);
        connect(m_showTexture, &QAction::toggled, this, [this]() { Refresh(); });
        connect(addAction(editorIcons + QStringLiteral("AutoFit.svg"), tr("Frame"), tr("Frame the selection, or everything when nothing is selected. [F]")),
            &QAction::triggered, this, [this]() { m_canvas->FrameSelection(); });

        layout->addWidget(m_canvas, 1);

        // Status on the left, the last operation's result on the right, on one line.
        auto* footer = new QHBoxLayout();
        footer->setContentsMargins(6, 3, 6, 3);
        m_status = new QLabel(this);
        m_message = new QLabel(this);
        m_message->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_message->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred); // long messages clip instead of widening the pane
        footer->addWidget(m_status);
        footer->addWidget(m_message, 1);
        layout->addLayout(footer);

        m_canvas->m_onEdit = [this](const AZStd::vector<UvChange>& changes, const bool final) { ApplyEdit(changes, final); };
        m_canvas->m_onStatus = [this](const QString& message) { m_status->setText(message); };

        auto* refresh = new QTimer(this);
        connect(refresh, &QTimer::timeout, this, [this]() { Refresh(); });
        refresh->start(150);
        Refresh();

        // The face under the viewport cursor, picked out in the UVs; quicker than the model refresh so it tracks the mouse.
        auto* hover = new QTimer(this);
        connect(hover, &QTimer::timeout, this, [this]()
        {
            Api::FaceHandles faces;
            if (m_pair.GetEntityId().IsValid())
            {
                EditorWhiteBoxTransformModeRequestBus::EventResult(faces, m_pair, &EditorWhiteBoxTransformModeRequests::GetHoveredFaces);
            }
            m_canvas->SetHoveredFaces(faces);
        });
        hover->start(50);

        AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            editorEntityContextId, &AzToolsFramework::EditorEntityContextRequests::GetEditorEntityContextId);
        AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusConnect(editorEntityContextId);
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusConnect(editorEntityContextId);
        EnsureEnabledInComponentMode(); // docked at startup, the pane may already be disabled
    }

    WhiteBoxUvEditorPane::~WhiteBoxUvEditorPane()
    {
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
    }

    void WhiteBoxUvEditorPane::RefreshTexture(const WhiteBoxMesh& mesh)
    {
        const Api::FaceHandles& faces = m_canvas->Model().m_faces;
        auto* component = FindWhiteBoxComponent(m_pair);
        if (!m_showTexture->isChecked() || faces.empty() || component == nullptr)
        {
            m_canvas->SetBackground(QImage());
            return;
        }
        // The first viewed face's material: its own override, else the entity's, else the built-in one.
        AZ::Data::AssetId material = Api::FaceMaterial(mesh, faces.front());
        if (!material.IsValid())
        {
            material = component->GetDefaultMaterialAssetId();
            if (!material.IsValid() && !component->GetMaterialUseTexture())
            {
                m_canvas->SetBackground(QImage()); // the built-in material with its texture switched off
                return;
            }
        }
        const UvTextureCache::Result texture = m_textures.Find(material);
        if (!texture.m_pending)
        {
            m_canvas->SetBackground(texture.m_image);
            m_showTexture->setToolTip(
                QStringLiteral("<b>%1</b><br>%2").arg(tr("Show Texture"), texture.m_image.isNull() ? tr("This material has no base colour texture to show.") : texture.m_name));
        }
    }

    void WhiteBoxUvEditorPane::DisplayViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (!isVisible() || !m_canvas->HasSelection() || !m_pair.GetEntityId().IsValid())
        {
            return;
        }
        auto* component = FindWhiteBoxComponent(m_pair);
        const WhiteBoxMesh* mesh = component != nullptr ? component->GetWhiteBoxMesh() : nullptr;
        if (mesh == nullptr)
        {
            return;
        }
        const auto vertexCount = static_cast<int>(Api::MeshVertexCount(*mesh));
        const auto faceCount = static_cast<int>(Api::MeshFaceCount(*mesh));
        const auto position = [mesh](const int vertex) { return Api::VertexPosition(*mesh, Api::VertexHandle{ vertex }); };

        // Drawn in the same editing space as the component mode (entity and active layer transform).
        debugDisplay.PushMatrix(EditorSpaceFromLocal(m_pair));
        const AZ::Color highlight(1.0f, 0.63f, 0.16f, 1.0f);

        // Selected faces as a translucent film lifted just off the surface, so it does not fight the mesh.
        debugDisplay.DepthTestOn();
        debugDisplay.SetColor(AZ::Color(1.0f, 0.63f, 0.16f, 0.35f));
        for (const auto face : m_canvas->SelectedFaces())
        {
            if (!face.IsValid() || face.Index() >= faceCount)
            {
                continue;
            }
            const auto corners = Api::FaceVertexPositions(*mesh, face);
            const AZ::Vector3 lift = Api::FaceNormal(*mesh, face) * 0.002f;
            debugDisplay.DrawTri(corners[0] + lift, corners[1] + lift, corners[2] + lift);
        }

        // Edges and corners on top of everything, so a selection behind the mesh is still visible.
        debugDisplay.DepthTestOff();
        debugDisplay.SetColor(highlight);
        debugDisplay.SetLineWidth(3.0f);
        for (const auto& edge : m_canvas->SelectedMeshEdges())
        {
            if (edge.first >= 0 && edge.first < vertexCount && edge.second >= 0 && edge.second < vertexCount)
            {
                debugDisplay.DrawLine(position(edge.first), position(edge.second));
            }
        }
        debugDisplay.SetLineWidth(1.0f);
        for (const int vertex : m_canvas->SelectedMeshVertices())
        {
            if (vertex >= 0 && vertex < vertexCount)
            {
                debugDisplay.DrawBall(position(vertex), 0.03f);
            }
        }
        debugDisplay.DepthTestOn();
        debugDisplay.PopMatrix();
    }

    void WhiteBoxUvEditorPane::changeEvent(QEvent* event)
    {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::EnabledChange && !isEnabled())
        {
            EnsureEnabledInComponentMode();
        }
    }

    void WhiteBoxUvEditorPane::OnEditorModeActivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
        const AzToolsFramework::ViewportEditorMode mode)
    {
        if (mode == AzToolsFramework::ViewportEditorMode::Component)
        {
            EnsureEnabledInComponentMode();
        }
    }

    void WhiteBoxUvEditorPane::EnsureEnabledInComponentMode()
    {
        // Deferred so it runs after whatever disabled the dock panes has finished.
        QTimer::singleShot(
            0, this,
            [this]()
            {
                namespace Cmf = AzToolsFramework::ComponentModeFramework;
                bool inComponentMode = false;
                Cmf::ComponentModeSystemRequestBus::BroadcastResult(inComponentMode, &Cmf::ComponentModeSystemRequests::InComponentMode);
                if (!inComponentMode)
                {
                    return; // outside component mode (game or simulation) the disable stands
                }
                // Whichever ancestor the editor disabled (the pane or its dock), and the grey effect it dims them with.
                for (QWidget* widget = this; widget != nullptr; widget = widget->parentWidget())
                {
                    if (!widget->isEnabled())
                    {
                        widget->setEnabled(true);
                    }
                    if (widget->graphicsEffect() != nullptr)
                    {
                        widget->setGraphicsEffect(nullptr);
                    }
                }
                for (QWidget* child : findChildren<QWidget*>())
                {
                    if (child->graphicsEffect() != nullptr)
                    {
                        child->setGraphicsEffect(nullptr);
                    }
                }
            });
    }

    void WhiteBoxUvEditorPane::Refresh()
    {
        // Mid-drag the canvas already holds the newest UVs; rebuilding would fight the gesture.
        if (m_canvas->IsDragging() || m_editing)
        {
            return;
        }
        m_pair = SelectedWhiteBox();
        auto* component = m_pair.GetEntityId().IsValid() ? FindWhiteBoxComponent(m_pair) : nullptr;
        const WhiteBoxMesh* mesh = component != nullptr ? component->GetWhiteBoxMesh() : nullptr;
        Api::PolygonHandles polygons;
        if (mesh != nullptr)
        {
            EditorWhiteBoxTransformModeRequestBus::EventResult(
                polygons, m_pair, &EditorWhiteBoxTransformModeRequests::GetSelectedPolygons);
        }
        m_canvas->SetModel(mesh != nullptr ? BuildUvModel(*mesh, polygons, m_canvas->DetachedCorners()) : UvModel{});
        if (mesh != nullptr)
        {
            RefreshTexture(*mesh);
        }
        else
        {
            m_canvas->SetBackground(QImage());
        }
    }

    void WhiteBoxUvEditorPane::ApplyLayout(const AZStd::vector<UvChange>& changes, const QString& done)
    {
        if (changes.empty())
        {
            m_message->setText(tr("Select polygons in Transform mode first."));
            return;
        }
        ApplyEdit(changes, true);
        Refresh();
        m_canvas->FrameAll();
        m_message->setText(done);
    }

    void WhiteBoxUvEditorPane::ApplyEdit(const AZStd::vector<UvChange>& changes, const bool final)
    {
        auto* component = FindWhiteBoxComponent(m_pair);
        WhiteBoxMesh* mesh = component != nullptr ? component->GetWhiteBoxMesh() : nullptr;
        if (mesh == nullptr)
        {
            m_editing = false;
            return;
        }
        if (!m_editing)
        {
            // A parametric layer would regenerate over the edit, so it is frozen before the first change lands.
            m_editing = true;
            component->BakeParametricLayer(component->GetActiveLayerIndex());
        }
        const auto halfedgeCount = Api::MeshHalfedgeCount(*mesh);
        for (const auto& change : changes)
        {
            // The mesh may have changed under the view (undo); a corner that no longer exists is skipped.
            if (change.first.IsValid() && static_cast<AZ::u64>(change.first.Index()) < halfedgeCount)
            {
                Api::SetHalfedgeManualUv(*mesh, change.first, change.second);
            }
        }
        if (!final)
        {
            // Mid-drag only the picture updates; the collider and game bake wait for the gesture to end.
            if (component->AssetInUse())
            {
                EditorWhiteBoxComponentNotificationBus::Event(m_pair, &EditorWhiteBoxComponentNotifications::OnWhiteBoxMeshModified);
            }
            else
            {
                component->RebuildWhiteBoxDeferred();
            }
            return;
        }
        m_editing = false;
        AzToolsFramework::ScopedUndoBatch undoBatch("White Box UV Edit");
        component->SerializeWhiteBox();
        EditorWhiteBoxComponentNotificationBus::Event(m_pair, &EditorWhiteBoxComponentNotifications::OnWhiteBoxMeshModified);
        undoBatch.MarkEntityDirty(m_pair.GetEntityId());
    }
} // namespace WhiteBox
