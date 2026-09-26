/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Tools/WhiteBoxUvEditorPane.h"
#include "Rendering/WhiteBoxMaterialSlot.h"

#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include "EditorWhiteBoxComponent.h"
#include "SubComponentModes/EditorWhiteBoxTransformModeBus.h"
#include "Util/WhiteBoxEditorUtil.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/MathUtils.h>
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
#include <QComboBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QStringList>
#include <QSignalBlocker>
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
        m_textureLayer = new QComboBox(toolbar);
        m_textureLayer->setToolTip(tr("Which layer's base colour texture to draw behind the UVs."));
        m_textureLayer->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        connect(m_textureLayer, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { Refresh(); });
        m_textureLayerAction = toolbar->addWidget(m_textureLayer);
        m_textureLayerAction->setVisible(false);
        m_trimToggle = addAction(whiteBoxIcons + QStringLiteral("UvTrim.svg"), tr("Trim Sheet"),
            tr("Show the viewed material's trim bands and the panel to set them up and fit selected islands into a band."));
        m_trimToggle->setCheckable(true);
        connect(m_trimToggle, &QAction::toggled, this, [this](const bool on)
        {
            m_trimPanel->setVisible(on);
            RefreshTrim();
        });
        m_showTexture = addAction(editorIcons + QStringLiteral("Material.svg"), tr("Show Texture"),
            tr("Draw the faces' base colour texture behind the UVs, repeating outside the unit square as it does on the mesh."));
        m_showTexture->setCheckable(true);
        m_showTexture->setChecked(true);
        connect(m_showTexture, &QAction::toggled, this, [this]() { Refresh(); });
        connect(addAction(editorIcons + QStringLiteral("AutoFit.svg"), tr("Frame"), tr("Frame the selection, or everything when nothing is selected. [F]")),
            &QAction::triggered, this, [this]() { m_canvas->FrameSelection(); });

        // Trim sheet panel, under the toolbar while the toggle is on.
        m_trimPanel = new QWidget(this);
        auto* trimLayout = new QHBoxLayout(m_trimPanel);
        trimLayout->setContentsMargins(6, 3, 6, 3);
        trimLayout->addWidget(new QLabel(tr("Bands"), m_trimPanel));
        m_trimEdges = new QLineEdit(m_trimPanel);
        m_trimEdges->setPlaceholderText(tr("V edges, top to bottom: 0, 0.25, 0.5, 1"));
        m_trimEdges->setToolTip(tr("Where the trim sheet's bands start and end, from 0 at the top of the texture to 1 at the bottom."));
        trimLayout->addWidget(m_trimEdges, 1);
        connect(m_trimEdges, &QLineEdit::editingFinished, this, [this]()
        {
            AZStd::vector<float> edges;
            for (const QString& part : m_trimEdges->text().split(QRegularExpression(QStringLiteral("[,;\\s]+")), Qt::SkipEmptyParts))
            {
                bool number = false;
                const float value = part.toFloat(&number);
                if (number)
                {
                    edges.push_back(value);
                }
            }
            SetTrimEdges(edges);
        });
        m_trimEvenCount = new QSpinBox(m_trimPanel);
        m_trimEvenCount->setRange(1, 64);
        m_trimEvenCount->setValue(4);
        m_trimEvenCount->setToolTip(tr("How many equal bands Even makes."));
        trimLayout->addWidget(m_trimEvenCount);
        auto* even = new QPushButton(tr("Even"), m_trimPanel);
        even->setToolTip(tr("Split the texture into equal bands."));
        connect(even, &QPushButton::clicked, this, [this]()
        {
            AZStd::vector<float> edges;
            const int count = m_trimEvenCount->value();
            for (int i = 0; i <= count; ++i)
            {
                edges.push_back(static_cast<float>(i) / static_cast<float>(count));
            }
            SetTrimEdges(edges);
        });
        trimLayout->addWidget(even);
        m_trimBand = new QComboBox(m_trimPanel);
        m_trimBand->setToolTip(tr("The band Fit puts the selection into."));
        connect(m_trimBand, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { RefreshTrim(); });
        trimLayout->addWidget(m_trimBand);
        trimLayout->addWidget(new QLabel(tr("Inset"), m_trimPanel));
        m_trimInset = new QDoubleSpinBox(m_trimPanel);
        m_trimInset->setRange(0.0, 0.05);
        m_trimInset->setSingleStep(0.001);
        m_trimInset->setDecimals(3);
        m_trimInset->setValue(0.002);
        m_trimInset->setToolTip(tr("Keep this far inside the band's top and bottom, so neighbouring trims do not bleed in."));
        trimLayout->addWidget(m_trimInset);
        auto* fit = new QPushButton(tr("Fit"), m_trimPanel);
        fit->setToolTip(tr("Fit each selected island (or all in view) into the band: long side along U, height filling the band."));
        connect(fit, &QPushButton::clicked, this, [this]()
        {
            auto* component = FindWhiteBoxComponent(m_pair);
            const AZStd::vector<float> edges =
                component != nullptr && m_viewMaterialKnown ? component->GetTrimEdges(m_viewMaterial) : AZStd::vector<float>{};
            const int band = m_trimBand->currentIndex();
            if (band < 0 || band + 1 >= static_cast<int>(edges.size()))
            {
                m_message->setText(tr("Set up the trim bands first."));
                return;
            }
            if (const WhiteBoxMesh* mesh = component != nullptr ? component->GetWhiteBoxMesh() : nullptr)
            {
                ApplyLayout(
                    UvOps::FitToBand(*mesh, m_canvas->TargetFaces(), edges[band], edges[band + 1], static_cast<float>(m_trimInset->value())),
                    tr("Fitted to band %1.").arg(band + 1));
            }
        });
        trimLayout->addWidget(fit);
        m_trimPanel->setVisible(false);
        layout->addWidget(m_trimPanel);

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
        if (faces.empty() || component == nullptr)
        {
            m_canvas->SetBackground(QImage());
            m_textureLayerAction->setVisible(false);
            m_viewMaterialKnown = false;
            RefreshTrim();
            return;
        }
        // The first viewed face's material: its own, else the entity's, else the built-in one.
        AZ::Data::AssetId material = Api::FaceMaterial(mesh, faces.front());
        if (!material.IsValid())
        {
            material = component->GetDefaultMaterialAssetId();
        }
        // A Material component on the entity may swap that slot's material or override its textures, as the renderer does:
        // the most specific entry that yields an instance wins (this LOD's slot, the slot, then the whole model).
        const AZ::u32 slot = MaterialSlotStableId(material, Api::FacePaintColor(mesh, faces.front()));
        AZ::Render::MaterialAssignmentMap assignments;
        AZ::Render::MaterialComponentRequestBus::EventResult(
            assignments, m_pair.GetEntityId(), &AZ::Render::MaterialComponentRequests::GetMaterialMapCopy);
        const UvTextureCache::PropertyOverrides* overrides = nullptr;
        for (const AZ::Render::MaterialAssignmentId& id :
             { AZ::Render::MaterialAssignmentId::CreateFromLodAndStableId(0, slot), AZ::Render::MaterialAssignmentId::CreateFromStableIdOnly(slot),
               AZ::Render::MaterialAssignmentId::CreateDefault() })
        {
            const auto it = assignments.find(id);
            if (it == assignments.end())
            {
                continue;
            }
            const bool swapsAsset = it->second.m_materialAsset.GetId().IsValid();
            // A whole-model entry without an asset has nothing to build an instance from.
            if (!swapsAsset && (id.IsDefault() || it->second.m_propertyOverrides.empty()))
            {
                continue;
            }
            if (swapsAsset)
            {
                material = it->second.m_materialAsset.GetId();
            }
            overrides = &it->second.m_propertyOverrides;
            break;
        }
        m_viewMaterial = material;
        m_viewMaterialKnown = true;
        RefreshTrim();
        if (!m_showTexture->isChecked())
        {
            m_canvas->SetBackground(QImage());
            m_textureLayerAction->setVisible(false);
            return;
        }
        if (!material.IsValid() && !component->GetMaterialUseTexture())
        {
            m_canvas->SetBackground(QImage()); // the built-in material with its texture switched off
            m_textureLayerAction->setVisible(false);
            return;
        }
        const UvTextureCache::Result found = m_textures.Find(material, overrides);
        if (found.m_pending)
        {
            return;
        }
        // Refill the layer list only when it changes, keeping the chosen layer by name across materials.
        QStringList names;
        for (const UvTextureCache::Texture& texture : found.m_textures)
        {
            names.append(texture.m_layer);
        }
        if (names != m_textureLayerNames)
        {
            const QString chosen = m_textureLayer->currentText();
            const QSignalBlocker block(m_textureLayer);
            m_textureLayer->clear();
            m_textureLayer->addItems(names);
            m_textureLayer->setCurrentIndex(AZStd::max(0, names.indexOf(chosen)));
            m_textureLayerNames = names;
        }
        m_textureLayerAction->setVisible(names.size() > 1);
        const int index = m_textureLayer->currentIndex();
        const UvTextureCache::Texture* texture =
            index >= 0 && index < static_cast<int>(found.m_textures.size()) ? &found.m_textures[index] : nullptr;
        m_canvas->SetBackground(texture != nullptr ? texture->m_image : QImage());
        m_showTexture->setToolTip(QStringLiteral("<b>%1</b><br>%2").arg(
            tr("Show Texture"), texture == nullptr ? tr("This material has no base colour texture to show.") : texture->m_name));
    }

    void WhiteBoxUvEditorPane::RefreshTrim()
    {
        auto* component = FindWhiteBoxComponent(m_pair);
        const AZStd::vector<float> edges =
            component != nullptr && m_viewMaterialKnown ? component->GetTrimEdges(m_viewMaterial) : AZStd::vector<float>{};
        // The text follows the stored edges unless it is being typed in.
        QStringList parts;
        for (const float edge : edges)
        {
            parts.append(QString::number(edge, 'g', 4));
        }
        if (!m_trimEdges->hasFocus())
        {
            m_trimEdges->setText(parts.join(QStringLiteral(", ")));
        }
        QStringList bands;
        for (size_t band = 0; band + 1 < edges.size(); ++band)
        {
            bands.append(tr("Band %1  (%2 - %3)").arg(band + 1).arg(edges[band], 0, 'g', 3).arg(edges[band + 1], 0, 'g', 3));
        }
        QStringList shown;
        for (int i = 0; i < m_trimBand->count(); ++i)
        {
            shown.append(m_trimBand->itemText(i));
        }
        if (bands != shown)
        {
            const int kept = m_trimBand->currentIndex();
            const QSignalBlocker block(m_trimBand);
            m_trimBand->clear();
            m_trimBand->addItems(bands);
            m_trimBand->setCurrentIndex(bands.isEmpty() ? -1 : AZ::GetClamp(kept, 0, static_cast<int>(bands.size()) - 1));
        }
        m_canvas->SetTrimBands(m_trimToggle->isChecked() ? edges : AZStd::vector<float>{}, m_trimBand->currentIndex());
    }

    void WhiteBoxUvEditorPane::SetTrimEdges(const AZStd::vector<float>& edges)
    {
        auto* component = FindWhiteBoxComponent(m_pair);
        if (component == nullptr || !m_viewMaterialKnown)
        {
            m_message->setText(tr("Select polygons in Transform mode first; the bands belong to their material."));
            return;
        }
        component->SetTrimEdges(m_viewMaterial, edges);
        m_trimEdges->clearFocus();
        RefreshTrim();
        m_message->setText(edges.size() >= 2 ? tr("Trim bands set.") : tr("Trim bands cleared."));
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
