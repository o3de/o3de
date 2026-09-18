/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Tools/WhiteBoxPaneWidget.h"

#include "EditorWhiteBoxComponent.h"
#include "EditorWhiteBoxComponentModeBus.h"
#include "Tools/WhiteBoxLayerUtil.h"
#include "SubComponentModes/EditorWhiteBoxTransformModeBus.h"
#include <AzToolsFramework/UI/PropertyEditor/PropertyAssetCtrl.hxx>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Quaternion.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDropEvent>
#include <QEvent>
#include <QGraphicsEffect>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

namespace WhiteBox
{
    namespace
    {
        //! Every entity in the level that has an Editor White Box component.
        AZStd::vector<AZ::EntityId> CollectWhiteBoxEntities()
        {
            AZStd::vector<AZ::EntityId> entityIds;
            auto callback = [&entityIds](AZ::Entity* entity)
            {
                if (entity != nullptr && entity->FindComponent<EditorWhiteBoxComponent>() != nullptr)
                {
                    entityIds.push_back(entity->GetId());
                }
            };
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::EnumerateEntities, callback);
            return entityIds;
        }

        AZ::Entity* FindEntity(const AZ::EntityId entityId)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(
                entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
            return entity;
        }

        QString EntityDisplayName(const AZ::EntityId entityId)
        {
            if (AZ::Entity* entity = FindEntity(entityId))
            {
                return QString::fromUtf8(entity->GetName().c_str());
            }
            return QStringLiteral("<unknown>");
        }

        void SetColorButtonSwatch(QPushButton* button, const QColor& color)
        {
            button->setStyleSheet(QStringLiteral("background-color: %1;").arg(color.name()));
        }

        QColor ToQColor(const AZ::Vector3& tint)
        {
            return QColor::fromRgbF(tint.GetX(), tint.GetY(), tint.GetZ());
        }

        QDoubleSpinBox* MakeSpin(double min, double max, double step, int decimals = 3)
        {
            auto* spin = new QDoubleSpinBox();
            spin->setRange(min, max);
            spin->setSingleStep(step);
            spin->setDecimals(decimals);
            return spin;
        }

        QWidget* MakeVec3Row(QDoubleSpinBox* (&spins)[3], double min, double max, double step)
        {
            auto* row = new QWidget();
            auto* layout = new QHBoxLayout(row);
            layout->setContentsMargins(0, 0, 0, 0);
            for (int i = 0; i < 3; ++i)
            {
                spins[i] = MakeSpin(min, max, step);
                layout->addWidget(spins[i]);
            }
            return row;
        }

        //! The layer list, with drag-and-drop reordering.
        //!
        //! The rows are NOT moved by the view: letting Qt do an InternalMove would edit the
        //! item model behind the component's back (and fire itemChanged mid-move, writing a
        //! layer's name/visibility onto the wrong slot). Instead the drop is intercepted, the
        //! from/to rows are computed and handed to the owner, which reorders the component's
        //! layer list and rebuilds the widget from it - the component stays the source of truth.
        //! No signals/slots of its own, so it needs no Q_OBJECT / moc.
        class LayerListWidget : public QListWidget
        {
        public:
            using QListWidget::QListWidget;

            //! Called with (fromRow, toRow) when the user drops a dragged layer.
            AZStd::function<void(int, int)> m_onRowMoved;

        protected:
            void dropEvent(QDropEvent* event) override
            {
                const int from = currentRow();

                // Where the drop indicator sits translates to an INSERTION index in the list.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                const QPoint dropPos = event->position().toPoint();
#else
                const QPoint dropPos = event->pos();
#endif
                int insertIndex = count();
                if (const QModelIndex index = indexAt(dropPos); index.isValid())
                {
                    switch (dropIndicatorPosition())
                    {
                    case QAbstractItemView::AboveItem:
                        insertIndex = index.row();
                        break;
                    case QAbstractItemView::BelowItem:
                    case QAbstractItemView::OnItem:
                        insertIndex = index.row() + 1;
                        break;
                    default:
                        break;
                    }
                }

                // Removing the dragged row first shifts everything after it down by one.
                const int to = (insertIndex > from) ? insertIndex - 1 : insertIndex;

                // Swallow the event so the base class never touches the model.
                event->setDropAction(Qt::IgnoreAction);
                event->accept();

                if (m_onRowMoved && from >= 0 && to >= 0 && to != from)
                {
                    m_onRowMoved(from, to);
                }
            }
        };
    } // namespace

    WhiteBoxPaneWidget::WhiteBoxPaneWidget(QWidget* parent)
        : QScrollArea(parent)
    {
        auto* container = new QWidget();
        auto* mainLayout = new QVBoxLayout(container);
        mainLayout->setContentsMargins(4, 4, 4, 4);
        mainLayout->setSpacing(6);

        m_layerGizmo = AZStd::make_unique<WhiteBoxLayerGizmo>();
        m_entityGizmo = AZStd::make_unique<WhiteBoxEntityGizmo>();

        mainLayout->addWidget(BuildEntitySection());
        mainLayout->addWidget(BuildEntityTransformSection());
        mainLayout->addWidget(BuildModeSection());
        mainLayout->addWidget(BuildVertexPaintSection());
        mainLayout->addWidget(BuildShapeSection());
        mainLayout->addWidget(BuildLayersSection());
        mainLayout->addWidget(BuildDrawSection());
        mainLayout->addWidget(BuildCubeStampSection());
        mainLayout->addWidget(BuildBooleanSection());
        mainLayout->addWidget(BuildMaterialSection());
        mainLayout->addWidget(BuildMeshOpsSection());
        mainLayout->addStretch();

        setWidget(container);
        setWidgetResizable(true);

        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
        AzToolsFramework::EntityCompositionNotificationBus::Handler::BusConnect();

        AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            editorEntityContextId, &AzToolsFramework::EditorEntityContextRequests::GetEditorEntityContextId);
        AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusConnect(editorEntityContextId);

        RefreshEntityList();
        RefreshFromComponent();
    }

    WhiteBoxPaneWidget::~WhiteBoxPaneWidget()
    {
        RemoveEntityGizmoCluster();
        m_entityGizmo.reset();
        m_layerGizmo.reset();
        EditorWhiteBoxComponentNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
        AzToolsFramework::EntityCompositionNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
    }

    // ---- UI construction ------------------------------------------------------------------

    QWidget* WhiteBoxPaneWidget::BuildEntitySection()
    {
        auto* group = new QGroupBox(tr("Entity"));
        auto* layout = new QVBoxLayout(group);

        auto* comboRow = new QHBoxLayout();
        m_entityCombo = new QComboBox();
        m_entityCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        comboRow->addWidget(m_entityCombo);
        auto* refreshButton = new QPushButton(tr("Refresh"));
        refreshButton->setToolTip(tr("Re-scan the level for entities with a White Box component."));
        comboRow->addWidget(refreshButton);
        layout->addLayout(comboRow);

        auto* buttonRow = new QHBoxLayout();
        m_newEntityButton = new QPushButton(tr("New Entity"));
        m_newEntityButton->setToolTip(tr("Create a new entity with a White Box component and start editing it."));
        m_newChildButton = new QPushButton(tr("New Layer (Child)"));
        m_newChildButton->setToolTip(
            tr("Create a child entity with its own White Box component under the current one and switch edit "
               "focus to it (same as the old component button)."));
        m_editButton = new QPushButton(tr("Edit"));
        m_editButton->setToolTip(tr("Select the entity and enter White Box edit mode."));
        m_doneButton = new QPushButton(tr("Done"));
        m_doneButton->setToolTip(tr("Leave White Box edit mode."));
        buttonRow->addWidget(m_newEntityButton);
        buttonRow->addWidget(m_newChildButton);
        buttonRow->addWidget(m_editButton);
        buttonRow->addWidget(m_doneButton);
        layout->addLayout(buttonRow);

        connect(
            m_entityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &WhiteBoxPaneWidget::OnEntityComboChanged);
        connect(refreshButton, &QPushButton::clicked, this,
            [this]()
            {
                RefreshEntityList();
                RefreshFromComponent();
            });
        connect(m_newEntityButton, &QPushButton::clicked, this, [this]() { CreateWhiteBoxEntity(false); });
        connect(m_newChildButton, &QPushButton::clicked, this, [this]() { CreateWhiteBoxEntity(true); });
        connect(m_editButton, &QPushButton::clicked, this,
            [this]()
            {
                const AZ::EntityId entityId = m_currentEntityId;
                if (!entityId.IsValid())
                {
                    return;
                }
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                    &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
                    AzToolsFramework::EntityIdList{ entityId });
                // Enter component mode on the next tick so the selection has fully applied first.
                AZ::TickBus::QueueFunction(
                    [entityId]()
                    {
                        if (AZ::Entity* entity = FindEntity(entityId))
                        {
                            if (auto* whiteBox = entity->FindComponent<EditorWhiteBoxComponent>())
                            {
                                whiteBox->EnterComponentMode();
                            }
                        }
                    });
            });
        connect(m_doneButton, &QPushButton::clicked, this,
            []()
            {
                namespace Cmf = AzToolsFramework::ComponentModeFramework;
                Cmf::ComponentModeSystemRequestBus::Broadcast(&Cmf::ComponentModeSystemRequests::EndComponentMode);
            });

        return group;
    }

    QWidget* WhiteBoxPaneWidget::BuildEntityTransformSection()
    {
        auto* group = new QGroupBox(tr("Entity Transform"));
        auto* layout = new QFormLayout(group);

        // Space the Position/Rotation values are shown and edited in. "Parent" is the entity's
        // local transform (what the Transform component shows); "World" is absolute.
        m_entitySpaceCombo = new QComboBox();
        m_entitySpaceCombo->addItem(tr("Parent (Local)"));
        m_entitySpaceCombo->addItem(tr("World"));
        layout->addRow(tr("Space"), m_entitySpaceCombo);

        layout->addRow(tr("Position"), MakeVec3Row(m_entityPos, -100000.0, 100000.0, 0.1));
        layout->addRow(tr("Rotation"), MakeVec3Row(m_entityRot, -3600.0, 3600.0, 1.0));
        m_entityScale = MakeSpin(0.001, 1000.0, 0.1);
        layout->addRow(tr("Uniform Scale"), m_entityScale);
        auto* nonUniformRow = MakeVec3Row(m_entityNonUniformScale, 0.001, 1000.0, 0.1);
        for (QDoubleSpinBox* spin : m_entityNonUniformScale)
        {
            spin->setToolTip(
                tr("Per-axis (non-uniform) scale for the whole entity. Setting a non-(1,1,1) value adds an "
                   "O3DE Non-Uniform Scale component if the entity doesn't already have one; it also drives the "
                   "transform Scale gizmo. Multiplies with Uniform Scale above."));
        }
        layout->addRow(tr("Non-Uniform Scale"), nonUniformRow);

        // Viewport gizmo driving the entity transform - available even during component mode
        // (when the editor's built-in entity gizmo is suppressed).
        auto* gizmoRow = new QWidget();
        auto* gizmoLayout = new QHBoxLayout(gizmoRow);
        gizmoLayout->setContentsMargins(0, 0, 0, 0);
        const auto makeEntityGizmoButton =
            [this, gizmoLayout](const QString& label, WhiteBoxEntityGizmo::Mode mode) -> QPushButton*
        {
            auto* button = new QPushButton(label);
            button->setCheckable(true);
            button->setAutoExclusive(true);
            gizmoLayout->addWidget(button);
            connect(button, &QPushButton::clicked, this,
                [this, mode]()
                {
                    m_entityGizmo->SetTarget(m_currentEntityId);
                    m_entityGizmo->SetMode(mode);
                });
            return button;
        };
        m_entityGizmoOff = makeEntityGizmoButton(tr("Off"), WhiteBoxEntityGizmo::Mode::None);
        m_entityGizmoMove = makeEntityGizmoButton(tr("Move"), WhiteBoxEntityGizmo::Mode::Translate);
        m_entityGizmoRotate = makeEntityGizmoButton(tr("Rotate"), WhiteBoxEntityGizmo::Mode::Rotate);
        m_entityGizmoScale = makeEntityGizmoButton(tr("Scale"), WhiteBoxEntityGizmo::Mode::Scale);
        m_entityGizmoOff->setChecked(true);
        layout->addRow(tr("Gizmo"), gizmoRow);
        m_entityGizmo->SetChangedCallback([this]() { RefreshFromComponent(); });

        connect(
            m_entitySpaceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int)
            {
                if (!m_updating)
                {
                    UpdateEntityTransformUi(); // re-read the values in the newly chosen space
                }
            });

        const auto applyEntityTransform = [this]()
        {
            if (m_updating || !m_currentEntityId.IsValid())
            {
                return;
            }
            const bool world = m_entitySpaceCombo->currentIndex() == 1;
            const AZ::Vector3 position(
                aznumeric_cast<float>(m_entityPos[0]->value()), aznumeric_cast<float>(m_entityPos[1]->value()),
                aznumeric_cast<float>(m_entityPos[2]->value()));
            const AZ::Quaternion rotation = AZ::Quaternion::CreateFromEulerAnglesDegrees(AZ::Vector3(
                aznumeric_cast<float>(m_entityRot[0]->value()), aznumeric_cast<float>(m_entityRot[1]->value()),
                aznumeric_cast<float>(m_entityRot[2]->value())));
            const float scale = aznumeric_cast<float>(m_entityScale->value());

            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Entity Transform");
            if (world)
            {
                AZ::TransformBus::Event(
                    m_currentEntityId, &AZ::TransformBus::Events::SetWorldTranslation, position);
                AZ::TransformBus::Event(
                    m_currentEntityId, &AZ::TransformBus::Events::SetWorldRotationQuaternion, rotation);
            }
            else
            {
                AZ::TransformBus::Event(
                    m_currentEntityId, &AZ::TransformBus::Events::SetLocalTranslation, position);
                AZ::TransformBus::Event(
                    m_currentEntityId, &AZ::TransformBus::Events::SetLocalRotationQuaternion, rotation);
            }
            AZ::TransformBus::Event(m_currentEntityId, &AZ::TransformBus::Events::SetLocalUniformScale, scale);
            undoBatch.MarkEntityDirty(m_currentEntityId);
        };
        for (QDoubleSpinBox* spin :
             { m_entityPos[0], m_entityPos[1], m_entityPos[2], m_entityRot[0], m_entityRot[1], m_entityRot[2],
               m_entityScale })
        {
            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, applyEntityTransform);
        }

        // Non-uniform scale goes through its own component, added the first time it is not (1,1,1).
        const auto applyNonUniformScale = [this]()
        {
            if (m_updating || !m_currentEntityId.IsValid())
            {
                return;
            }
            const AZ::Vector3 scale(
                aznumeric_cast<float>(m_entityNonUniformScale[0]->value()),
                aznumeric_cast<float>(m_entityNonUniformScale[1]->value()),
                aznumeric_cast<float>(m_entityNonUniformScale[2]->value()));

            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Non-Uniform Scale");
            if (!AZ::NonUniformScaleRequestBus::HasHandlers(m_currentEntityId) &&
                !scale.IsClose(AZ::Vector3::CreateOne()))
            {
                AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome outcome =
                    AZ::Failure(AZStd::string("uninitialized"));
                const AzToolsFramework::EntityIdList entities{ m_currentEntityId };
                const AZ::ComponentTypeList types{
                    azrtti_typeid<AzToolsFramework::Components::EditorNonUniformScaleComponent>()
                };
                AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
                    outcome, &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities, entities, types);
            }
            AZ::NonUniformScaleRequestBus::Event(m_currentEntityId, &AZ::NonUniformScaleRequests::SetScale, scale);
            undoBatch.MarkEntityDirty(m_currentEntityId);
        };
        for (QDoubleSpinBox* spin : m_entityNonUniformScale)
        {
            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, applyNonUniformScale);
        }

        return group;
    }

    QWidget* WhiteBoxPaneWidget::BuildModeSection()
    {
        auto* group = new QGroupBox(tr("Tool Mode"));
        auto* layout = new QHBoxLayout(group);

        const auto makeModeButton = [this, layout](const QString& label, SubMode subMode) -> QPushButton*
        {
            auto* button = new QPushButton(label);
            button->setCheckable(true);
            button->setAutoExclusive(true);
            layout->addWidget(button);
            connect(button, &QPushButton::clicked, this,
                [this, subMode]()
                {
                    EditorWhiteBoxComponent* component = CurrentComponent();
                    if (component == nullptr)
                    {
                        return;
                    }

                    namespace Cmf = AzToolsFramework::ComponentModeFramework;
                    bool inComponentMode = false;
                    Cmf::ComponentModeSystemRequestBus::BroadcastResult(
                        inComponentMode, &Cmf::ComponentModeSystemRequests::InComponentMode);

                    if (inComponentMode)
                    {
                        EditorWhiteBoxComponentModeRequestBus::Event(
                            AZ::EntityComponentIdPair(m_currentEntityId, component->GetId()),
                            &EditorWhiteBoxComponentModeRequests::SetSubMode, subMode);
                        return;
                    }

                    // Not editing yet: select the entity, enter White Box edit mode, then apply
                    // the requested sub-mode (deferred a tick so the mode exists first).
                    const AZ::EntityId entityId = m_currentEntityId;
                    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                        &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
                        AzToolsFramework::EntityIdList{ entityId });
                    AZ::TickBus::QueueFunction(
                        [entityId, subMode]()
                        {
                            if (AZ::Entity* entity = FindEntity(entityId))
                            {
                                if (auto* whiteBox = entity->FindComponent<EditorWhiteBoxComponent>())
                                {
                                    whiteBox->EnterComponentMode();
                                    EditorWhiteBoxComponentModeRequestBus::Event(
                                        AZ::EntityComponentIdPair(entityId, whiteBox->GetId()),
                                        &EditorWhiteBoxComponentModeRequests::SetSubMode, subMode);
                                }
                            }
                        });
                });
            return button;
        };

        m_modeSketch = makeModeButton(tr("Sketch"), SubMode::Default);
        m_modeEdgeRestore = makeModeButton(tr("Edge Restore"), SubMode::EdgeRestore);
        m_modeTransform = makeModeButton(tr("Transform"), SubMode::Transform);
        m_modeDrawShape = makeModeButton(tr("Draw Shape"), SubMode::DrawShape);
        m_modePaint = makeModeButton(tr("Vertex Paint"), SubMode::VertexPaint);
        m_modeTransform->setToolTip(tr(
            "Click a vertex, edge, or polygon to select it. Ctrl-click adds or removes elements of the same type. "
            "Selected elements transform together around their shared center."));

        return group;
    }

    QWidget* WhiteBoxPaneWidget::BuildShapeSection()
    {
        auto* group = new QGroupBox(tr("Default Shape"));
        auto* layout = new QFormLayout(group);

        m_defaultShapeCombo = new QComboBox();
        m_defaultShapeCombo->addItem(tr("Cube"), static_cast<int>(DefaultShapeType::Cube));
        m_defaultShapeCombo->addItem(tr("Tetrahedron"), static_cast<int>(DefaultShapeType::Tetrahedron));
        m_defaultShapeCombo->addItem(tr("Icosahedron"), static_cast<int>(DefaultShapeType::Icosahedron));
        m_defaultShapeCombo->addItem(tr("Cylinder"), static_cast<int>(DefaultShapeType::Cylinder));
        m_defaultShapeCombo->addItem(tr("Sphere"), static_cast<int>(DefaultShapeType::Sphere));
        m_defaultShapeCombo->addItem(tr("Mesh Asset"), static_cast<int>(DefaultShapeType::Asset));
        layout->addRow(tr("Shape"), m_defaultShapeCombo);

        connect(
            m_defaultShapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index)
            {
                if (m_updating)
                {
                    return;
                }
                const auto shape = static_cast<DefaultShapeType>(m_defaultShapeCombo->itemData(index).toInt());
                ModifyComponent(
                    "White Box Default Shape",
                    [shape](EditorWhiteBoxComponent* component) { component->SetDefaultShape(shape); });
            });

        return group;
    }

    QWidget* WhiteBoxPaneWidget::BuildLayersSection()
    {
        auto* group = new QGroupBox(tr("Layers"));
        auto* layout = new QVBoxLayout(group);

        auto* activeRow = new QFormLayout();
        m_activeLayerCombo = new QComboBox();
        m_activeLayerCombo->setToolTip(tr("Which layer edits (draw / stamp / carve) target."));
        activeRow->addRow(tr("Active Layer"), m_activeLayerCombo);
        layout->addLayout(activeRow);

        // Create Parametric Shape: one click adds a new parametric layer generating that shape
        // (made active + selected); its parameters are then edited live in "Selected Layer".
        auto* createGrid = new QGridLayout();
        const AZStd::pair<const char*, DrawShapeType> shapeDefs[] = {
            { "Box", DrawShapeType::Box },           { "Cylinder", DrawShapeType::Cylinder },
            { "Pyramid", DrawShapeType::Pyramid },   { "Cone", DrawShapeType::Cone },
            { "Sphere", DrawShapeType::Sphere },     { "Staircase", DrawShapeType::Staircase },
            { "Room", DrawShapeType::Room },
            { "Door", DrawShapeType::Door }, { "Circular Stairs", DrawShapeType::CircularStairs },
            { "Plane", DrawShapeType::Plane }, { "Torus", DrawShapeType::Torus }, { "Pipe", DrawShapeType::Pipe },
        };
        int shapeButtonIndex = 0;
        for (const auto& [label, shapeType] : shapeDefs)
        {
            auto* button = new QPushButton(tr(label));
            createGrid->addWidget(button, shapeButtonIndex / 3, shapeButtonIndex % 3);
            ++shapeButtonIndex;
            connect(button, &QPushButton::clicked, this,
                [this, shapeType]()
                {
                    ModifyComponent(
                        "Add Parametric Shape",
                        [shapeType](EditorWhiteBoxComponent* c) { c->AddParametricShapeLayer(shapeType); });
                    // Move the list selection to the freshly added (now active) layer.
                    if (EditorWhiteBoxComponent* component = CurrentComponent())
                    {
                        m_layerList->setCurrentRow(component->GetActiveLayerIndex());
                    }
                });
        }
        auto* createGroup = new QGroupBox(tr("Create Parametric Shape"));
        createGroup->setLayout(createGrid);
        layout->addWidget(createGroup);

        auto* buttonRow = new QHBoxLayout();
        auto* newLayerButton = new QPushButton(tr("New"));
        auto* deleteLayerButton = new QPushButton(tr("Delete"));
        auto* applyTransformButton = new QPushButton(tr("Apply Transform"));
        applyTransformButton->setToolTip(
            tr("Bake the active layer's Position/Rotation/Scale into its geometry and reset them to identity."));
        buttonRow->addWidget(newLayerButton);
        buttonRow->addWidget(deleteLayerButton);
        buttonRow->addWidget(applyTransformButton);
        layout->addLayout(buttonRow);

        auto* moveRow = new QHBoxLayout();
        auto* moveUpButton = new QPushButton(tr("Move Up"));
        auto* moveDownButton = new QPushButton(tr("Move Down"));
        const QString moveTip =
            tr("Reorder the selected layer. Layer order is the order the Combine modes accumulate "
               "in, so moving a layer restacks its boolean.");
        moveUpButton->setToolTip(moveTip);
        moveDownButton->setToolTip(moveTip);
        moveRow->addWidget(moveUpButton);
        moveRow->addWidget(moveDownButton);
        layout->addLayout(moveRow);

        auto* layerList = new LayerListWidget();
        m_layerList = layerList;
        m_layerList->setToolTip(
            tr("All layers, in combine order. Double-click to rename; use the checkbox to "
               "show/hide; drag a layer (or use Move Up / Move Down) to reorder."));
        // The list grows to show EVERY layer (the pane itself scrolls); no inner scrollbar.
        m_layerList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        // Drag to reorder. The drop is intercepted by LayerListWidget and applied to the
        // component (which owns the order); the widget is then rebuilt from it.
        m_layerList->setSelectionMode(QAbstractItemView::SingleSelection);
        m_layerList->setDragEnabled(true);
        m_layerList->viewport()->setAcceptDrops(true);
        m_layerList->setDropIndicatorShown(true);
        m_layerList->setDragDropMode(QAbstractItemView::InternalMove);
        m_layerList->setDefaultDropAction(Qt::MoveAction);
        // Rename on double-click / F2 only - a single click on an already-selected layer must
        // start a drag, not an inline rename.
        m_layerList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
        layerList->m_onRowMoved = [this](int from, int to)
        {
            MoveLayerRow(from, to);
        };
        layout->addWidget(m_layerList);

        m_layerMetaGroup = new QGroupBox(tr("Selected Layer"));
        auto* metaLayout = new QFormLayout(m_layerMetaGroup);
        m_layerTintButton = new QPushButton();
        m_layerTintButton->setToolTip(tr("Render colour for this layer (used when 'Use Global Tint' is off)."));
        metaLayout->addRow(tr("Tint"), m_layerTintButton);
        m_layerCombineCombo = new QComboBox();
        m_layerCombineCombo->addItem(tr("Separate"), static_cast<int>(LayerCombineMode::Separate));
        m_layerCombineCombo->addItem(tr("Union"), static_cast<int>(LayerCombineMode::Union));
        m_layerCombineCombo->addItem(tr("Subtract"), static_cast<int>(LayerCombineMode::Subtract));
        m_layerCombineCombo->addItem(tr("Intersect"), static_cast<int>(LayerCombineMode::Intersect));
        metaLayout->addRow(tr("Combine"), m_layerCombineCombo);
        m_layerInvertNormals = new QCheckBox(tr("Invert Normals"));
        m_layerInvertNormals->setToolTip(
            tr("Flip this layer's winding so it renders, collides and selects inside-out "
               "(e.g. a room built from an inverted box collides from the inside)."));
        metaLayout->addRow(QString(), m_layerInvertNormals);
        m_layerEdgesOnly = new QCheckBox(tr("Edges Only"));
        m_layerEdgesOnly->setToolTip(
            tr("Hide this layer's solid faces and draw only its edges (visual only - collision "
               "and selection are unaffected)."));
        metaLayout->addRow(QString(), m_layerEdgesOnly);
        m_layerCollision = new QCheckBox(tr("Collision"));
        m_layerCollision->setToolTip(tr("Include this layer in the physics collision mesh."));
        metaLayout->addRow(QString(), m_layerCollision);
        metaLayout->addRow(tr("Position"), MakeVec3Row(m_layerPos, -100000.0, 100000.0, 0.1));
        metaLayout->addRow(tr("Rotation"), MakeVec3Row(m_layerRot, -3600.0, 3600.0, 1.0));
        metaLayout->addRow(tr("Scale"), MakeVec3Row(m_layerScale, 0.001, 1000.0, 0.1));

        // Viewport gizmo for the selected layer's transform (same manipulators as the editor's
        // Transform component, but driving the layer's non-destructive Position/Rotation/Scale).
        auto* gizmoRow = new QWidget();
        auto* gizmoLayout = new QHBoxLayout(gizmoRow);
        gizmoLayout->setContentsMargins(0, 0, 0, 0);
        const auto makeGizmoButton =
            [this, gizmoLayout](const QString& label, WhiteBoxLayerGizmo::Mode mode) -> QPushButton*
        {
            auto* button = new QPushButton(label);
            button->setCheckable(true);
            button->setAutoExclusive(true);
            gizmoLayout->addWidget(button);
            connect(button, &QPushButton::clicked, this,
                [this, mode]()
                {
                    m_layerGizmo->SetTarget(m_currentEntityId, m_layerList->currentRow());
                    m_layerGizmo->SetMode(mode);
                });
            return button;
        };
        m_gizmoOff = makeGizmoButton(tr("Off"), WhiteBoxLayerGizmo::Mode::None);
        m_gizmoMove = makeGizmoButton(tr("Move"), WhiteBoxLayerGizmo::Mode::Translate);
        m_gizmoRotate = makeGizmoButton(tr("Rotate"), WhiteBoxLayerGizmo::Mode::Rotate);
        m_gizmoScale = makeGizmoButton(tr("Scale"), WhiteBoxLayerGizmo::Mode::Scale);
        m_gizmoOff->setChecked(true);
        metaLayout->addRow(tr("Gizmo"), gizmoRow);

        // Parametric shapes update live as their values change.
        m_shapeParamsGroup = new QGroupBox(tr("Shape Parameters"));
        auto* shapeLayout = new QFormLayout(m_shapeParamsGroup);
        m_shapeParamShape = new QComboBox();
        m_shapeParamShape->addItem(tr("Box"), static_cast<int>(DrawShapeType::Box));
        m_shapeParamShape->addItem(tr("Cylinder"), static_cast<int>(DrawShapeType::Cylinder));
        m_shapeParamShape->addItem(tr("Pyramid"), static_cast<int>(DrawShapeType::Pyramid));
        m_shapeParamShape->addItem(tr("Cone"), static_cast<int>(DrawShapeType::Cone));
        m_shapeParamShape->addItem(tr("Sphere"), static_cast<int>(DrawShapeType::Sphere));
        m_shapeParamShape->addItem(tr("Staircase"), static_cast<int>(DrawShapeType::Staircase));
        m_shapeParamShape->addItem(tr("Room"), static_cast<int>(DrawShapeType::Room));
        m_shapeParamShape->addItem(tr("Door"), static_cast<int>(DrawShapeType::Door));
        m_shapeParamShape->addItem(tr("Circular Stairs"), static_cast<int>(DrawShapeType::CircularStairs));
        m_shapeParamShape->addItem(tr("Plane"), static_cast<int>(DrawShapeType::Plane));
        m_shapeParamShape->addItem(tr("Torus"), static_cast<int>(DrawShapeType::Torus));
        m_shapeParamShape->addItem(tr("Pipe"), static_cast<int>(DrawShapeType::Pipe));
        shapeLayout->addRow(tr("Shape"), m_shapeParamShape);
        m_shapeParamWidth = MakeSpin(0.01, 10000.0, 0.1);
        shapeLayout->addRow(tr("Width"), m_shapeParamWidth);
        m_shapeParamDepth = MakeSpin(0.01, 10000.0, 0.1);
        m_shapeParamDepth->setToolTip(tr("Extent along local Y. Stretch a Sphere's Height (or Depth) to make a bullet."));
        shapeLayout->addRow(tr("Depth"), m_shapeParamDepth);
        m_shapeParamHeight = MakeSpin(0.01, 10000.0, 0.1);
        shapeLayout->addRow(tr("Height"), m_shapeParamHeight);
        m_shapeParamSides = new QSpinBox();
        m_shapeParamSides->setRange(3, 128);
        m_shapeParamSidesLabel = new QLabel(tr("Sides"));
        shapeLayout->addRow(m_shapeParamSidesLabel, m_shapeParamSides);
        m_shapeParamSteps = new QSpinBox();
        m_shapeParamSteps->setRange(1, 128);
        m_shapeParamStepsLabel = new QLabel(tr("Steps"));
        shapeLayout->addRow(m_shapeParamStepsLabel, m_shapeParamSteps);
        m_shapeParamStepsByHeight = new QCheckBox(tr("Divide By Step Height"));
        shapeLayout->addRow(QString(), m_shapeParamStepsByHeight);
        m_shapeParamStepHeight = MakeSpin(0.001, 10000.0, 0.05);
        m_shapeParamStepHeight->setToolTip(tr(
            "Derives the nearest whole step count from Height / Step Height (1?128 steps), "
            "then fits equal risers to the total height, like the staircase Draw tool."));
        m_shapeParamStepHeightLabel = new QLabel(tr("Step Height"));
        shapeLayout->addRow(m_shapeParamStepHeightLabel, m_shapeParamStepHeight);
        // Room-only controls. Width/Depth/Height above are the interior dimensions for a Room.
        m_shapeParamWallThickness = MakeSpin(0.001, 100.0, 0.05);
        m_shapeParamWallThickness->setToolTip(tr("Thickness of each wall leaf (inner and outer)."));
        m_shapeParamWallThicknessLabel = new QLabel(tr("Wall Thickness"));
        shapeLayout->addRow(m_shapeParamWallThicknessLabel, m_shapeParamWallThickness);
        m_shapeParamCavityGap = MakeSpin(0.0, 100.0, 0.05);
        m_shapeParamCavityGap->setToolTip(tr("Empty gap between the inner and outer wall leaves (0 = single solid wall)."));
        m_shapeParamCavityGapLabel = new QLabel(tr("Cavity Gap"));
        shapeLayout->addRow(m_shapeParamCavityGapLabel, m_shapeParamCavityGap);
        m_shapeParamFloor = new QCheckBox(tr("Floor"));
        m_shapeParamFloor->setToolTip(tr("Add a floor slab beneath the interior."));
        shapeLayout->addRow(QString(), m_shapeParamFloor);
        m_shapeParamCeiling = new QCheckBox(tr("Ceiling"));
        m_shapeParamCeiling->setToolTip(tr("Add a ceiling slab above the interior."));
        shapeLayout->addRow(QString(), m_shapeParamCeiling);
        m_shapeParamDoorFrame = new QCheckBox(tr("Doorway Frame (open)"));
        m_shapeParamDoorFrame->setToolTip(tr("Uncheck to create a solid door panel. Width and height describe the opening when checked."));
        shapeLayout->addRow(QString(), m_shapeParamDoorFrame);
        m_shapeParamArchHeight = MakeSpin(0.0, 10000.0, 0.05);
        m_shapeParamArchHeight->setToolTip(tr("Zero gives a rectangular door. Increase for an elliptical arch; half the width gives a semicircle."));
        shapeLayout->addRow(tr("Arch Height"), m_shapeParamArchHeight);
        m_shapeParamInnerRadius = MakeSpin(0.01, 10000.0, 0.1);
        shapeLayout->addRow(tr("Inner Radius"), m_shapeParamInnerRadius);
        m_shapeParamSweepAngle = MakeSpin(1.0, 360.0, 15.0);
        m_shapeParamSweepAngle->setToolTip(tr("Counterclockwise sweep around local Z. 360 degrees creates a full circular stairway."));
        shapeLayout->addRow(tr("Sweep Angle (degrees)"), m_shapeParamSweepAngle);
        m_shapeParamHoleRatio = MakeSpin(0.05, 0.95, 0.05);
        m_shapeParamHoleRatio->setToolTip(tr("Hole diameter as a fraction of the outer diameter. Higher values make a thinner wall or tube."));
        shapeLayout->addRow(tr("Hole Ratio"), m_shapeParamHoleRatio);
        m_shapeParamTubeSides = new QSpinBox();
        m_shapeParamTubeSides->setRange(MinTubeSides, MaxTubeSides);
        m_shapeParamTubeSides->setToolTip(
            tr("Segments around the torus tube's cross-section. A torus has Sides x Tube Sides quads, so this "
               "is the other half of its triangle count - raise Sides for a rounder ring, this for a rounder tube."));
        m_shapeParamTubeSidesLabel = new QLabel(tr("Tube Sides"));
        shapeLayout->addRow(m_shapeParamTubeSidesLabel, m_shapeParamTubeSides);
        m_bakeShapeButton = new QPushButton(tr("Bake To Mesh"));
        m_bakeShapeButton->setToolTip(
            tr("Freeze this parametric shape into an ordinary mesh layer so vertex-level edits are safe "
               "(the parameters stop driving it)."));
        shapeLayout->addRow(m_bakeShapeButton);
        metaLayout->addRow(m_shapeParamsGroup);

        // Reads the controls. Returns false when there is no layer to apply them to; `changed` reports
        // whether they actually differ from what the component already holds.
        const auto readShapeParams = [this](int& row, EditorWhiteBoxComponent::ShapeParams& params, bool& changed)
        {
            row = m_layerList->currentRow();
            if (row < 0)
            {
                return false;
            }
            params = EditorWhiteBoxComponent::ShapeParams{};
            params.m_shape = static_cast<DrawShapeType>(m_shapeParamShape->currentData().toInt());
            params.m_width = aznumeric_cast<float>(m_shapeParamWidth->value());
            params.m_depth = aznumeric_cast<float>(m_shapeParamDepth->value());
            params.m_height = aznumeric_cast<float>(m_shapeParamHeight->value());
            params.m_sides = m_shapeParamSides->value();
            params.m_steps = m_shapeParamSteps->value();
            params.m_stepsByHeight = m_shapeParamStepsByHeight->isChecked();
            params.m_stepHeight = aznumeric_cast<float>(m_shapeParamStepHeight->value());
            params.m_wallThickness = aznumeric_cast<float>(m_shapeParamWallThickness->value());
            params.m_cavityGap = aznumeric_cast<float>(m_shapeParamCavityGap->value());
            params.m_floor = m_shapeParamFloor->isChecked();
            params.m_ceiling = m_shapeParamCeiling->isChecked();
            params.m_doorFrame = m_shapeParamDoorFrame->isChecked();
            params.m_archHeight = aznumeric_cast<float>(m_shapeParamArchHeight->value());
            params.m_innerRadius = aznumeric_cast<float>(m_shapeParamInnerRadius->value());
            params.m_sweepAngle = aznumeric_cast<float>(m_shapeParamSweepAngle->value());
            params.m_holeRatio = aznumeric_cast<float>(m_shapeParamHoleRatio->value());
            params.m_tubeSides = m_shapeParamTubeSides->value();

            changed = true;
            if (auto* component = CurrentComponent())
            {
                const auto current = component->GetLayerShapeParams(row);
                changed = !(params.m_shape == current.m_shape && params.m_width == current.m_width &&
                    params.m_depth == current.m_depth && params.m_height == current.m_height &&
                    params.m_sides == current.m_sides && params.m_steps == current.m_steps &&
                    params.m_stepsByHeight == current.m_stepsByHeight && params.m_stepHeight == current.m_stepHeight &&
                    params.m_wallThickness == current.m_wallThickness && params.m_cavityGap == current.m_cavityGap &&
                    params.m_floor == current.m_floor && params.m_ceiling == current.m_ceiling &&
                    params.m_doorFrame == current.m_doorFrame && params.m_archHeight == current.m_archHeight &&
                    params.m_innerRadius == current.m_innerRadius && params.m_sweepAngle == current.m_sweepAngle &&
                    params.m_holeRatio == current.m_holeRatio && params.m_tubeSides == current.m_tubeSides);
            }
            return true;
        };

        // Intermediate values: regenerate geometry now; cook, bake and undo wait for the commit below.
        const auto previewShapeParams = [this, readShapeParams]()
        {
            if (m_updating)
            {
                return;
            }
            int row = -1;
            EditorWhiteBoxComponent::ShapeParams params;
            bool changed = false;
            if (!readShapeParams(row, params, changed) || !changed)
            {
                return;
            }
            if (auto* component = CurrentComponent())
            {
                component->SetLayerShapeParamsPreview(row, params);
                m_shapeParamsPendingCommit = true;
                m_shapeParamsPendingRow = row;
            }
            m_shapeParamCommitTimer->start();
        };

        // The real edit: full rebuild inside an undo batch. Runs once the value settles, or straight
        // away for the controls that cannot stream (the shape combo and the check boxes).
        const auto commitShapeParams = [this, readShapeParams]()
        {
            m_shapeParamCommitTimer->stop();
            if (m_updating)
            {
                return;
            }
            int row = -1;
            EditorWhiteBoxComponent::ShapeParams params;
            bool changed = false;
            if (!readShapeParams(row, params, changed))
            {
                return;
            }
            // A pending preview already wrote these values into the component, so `changed` is false by
            // then - the undo step and the deferred work still have to happen.
            if (!changed && !m_shapeParamsPendingCommit)
            {
                return;
            }
            if (m_shapeParamsPendingCommit && m_shapeParamsPendingRow >= 0)
            {
                row = m_shapeParamsPendingRow; // commit where the preview landed, not where the selection is now
            }
            m_shapeParamsPendingCommit = false;
            m_shapeParamsPendingRow = -1;
            ModifyComponent(
                "Edit Parametric Shape",
                [row, params](EditorWhiteBoxComponent* c) { c->SetLayerShapeParams(row, params); });
        };

        m_shapeParamCommitTimer = new QTimer(this);
        m_shapeParamCommitTimer->setSingleShot(true);
        m_shapeParamCommitTimer->setInterval(300);
        connect(m_shapeParamCommitTimer, &QTimer::timeout, this, commitShapeParams);

        connect(m_shapeParamShape, QOverload<int>::of(&QComboBox::currentIndexChanged), this, commitShapeParams);
        for (QDoubleSpinBox* spin :
             { m_shapeParamWidth, m_shapeParamDepth, m_shapeParamHeight, m_shapeParamWallThickness,
               m_shapeParamCavityGap, m_shapeParamArchHeight, m_shapeParamInnerRadius, m_shapeParamSweepAngle,
               m_shapeParamStepHeight, m_shapeParamHoleRatio })
        {
            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, previewShapeParams);
            connect(spin, &QAbstractSpinBox::editingFinished, this, commitShapeParams);
        }
        for (QSpinBox* spin : { m_shapeParamSides, m_shapeParamSteps, m_shapeParamTubeSides })
        {
            connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), this, previewShapeParams);
            connect(spin, &QAbstractSpinBox::editingFinished, this, commitShapeParams);
        }
        connect(m_shapeParamStepsByHeight, &QCheckBox::toggled, this, commitShapeParams);
        connect(m_shapeParamFloor, &QCheckBox::toggled, this, commitShapeParams);
        connect(m_shapeParamCeiling, &QCheckBox::toggled, this, commitShapeParams);
        connect(m_shapeParamDoorFrame, &QCheckBox::toggled, this, commitShapeParams);
        connect(m_bakeShapeButton, &QPushButton::clicked, this,
            [this]()
            {
                const int row = m_layerList->currentRow();
                if (row >= 0)
                {
                    ModifyComponent(
                        "Bake Parametric Shape",
                        [row](EditorWhiteBoxComponent* c) { c->BakeParametricLayer(row); });
                }
            });

        // When a gizmo drag finishes, reload the pane so the spin boxes show the new values.
        m_layerGizmo->SetChangedCallback([this]() { RefreshFromComponent(); });

        layout->addWidget(m_layerMetaGroup);

        connect(newLayerButton, &QPushButton::clicked, this,
            [this]() {
                ModifyComponent("White Box New Layer", [](EditorWhiteBoxComponent* c) { c->AddLayer(); });
            });
        connect(deleteLayerButton, &QPushButton::clicked, this,
            [this]() {
                ModifyComponent("White Box Delete Layer", [](EditorWhiteBoxComponent* c) { c->DeleteActiveLayer(); });
            });
        connect(applyTransformButton, &QPushButton::clicked, this,
            [this]()
            {
                ModifyComponent(
                    "White Box Apply Layer Transform",
                    [](EditorWhiteBoxComponent* c) { c->ApplyActiveLayerTransform(); });
            });
        connect(moveUpButton, &QPushButton::clicked, this,
            [this]()
            {
                const int row = m_layerList->currentRow();
                MoveLayerRow(row, row - 1);
            });
        connect(moveDownButton, &QPushButton::clicked, this,
            [this]()
            {
                const int row = m_layerList->currentRow();
                MoveLayerRow(row, row + 1);
            });
        connect(
            m_activeLayerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index)
            {
                if (m_updating || index < 0)
                {
                    return;
                }
                const int layerIndex = m_activeLayerCombo->itemData(index).toInt();
                ModifyComponent(
                    "White Box Active Layer",
                    [layerIndex](EditorWhiteBoxComponent* c) { c->SetActiveLayer(layerIndex); });
            });
        connect(m_layerList, &QListWidget::itemChanged, this,
            [this](QListWidgetItem* item)
            {
                if (m_updating || item == nullptr)
                {
                    return;
                }
                const int row = m_layerList->row(item);
                const AZStd::string name = item->text().toUtf8().constData();
                const bool visible = item->checkState() == Qt::Checked;
                ModifyComponent(
                    "White Box Layer Edit",
                    [row, name, visible](EditorWhiteBoxComponent* c)
                    {
                        EditorWhiteBoxComponent::LayerMeta meta = c->GetLayerMeta(row);
                        meta.m_name = name;
                        meta.m_visible = visible;
                        c->SetLayerMeta(row, meta);
                    });
            });
        connect(m_layerList, &QListWidget::currentRowChanged, this,
            [this](int row)
            {
                if (m_updating || row < 0)
                {
                    return;
                }
                // Selecting a layer in the list also makes it the EDIT TARGET (active layer),
                // so viewport tools immediately edit the layer whose data the pane shows.
                ModifyComponent(
                    "Switch White Box Layer",
                    [row](EditorWhiteBoxComponent* c) { c->SetActiveLayer(row); });
            });
        connect(m_layerTintButton, &QPushButton::clicked, this,
            [this]()
            {
                EditorWhiteBoxComponent* component = CurrentComponent();
                const int row = m_layerList->currentRow();
                if (component == nullptr || row < 0)
                {
                    return;
                }
                const QColor initial = ToQColor(component->GetLayerMeta(row).m_tint);
                const QColor picked = QColorDialog::getColor(initial, this, tr("Layer Tint"));
                if (!picked.isValid())
                {
                    return;
                }
                ModifyComponent(
                    "White Box Layer Tint",
                    [row, picked](EditorWhiteBoxComponent* c)
                    {
                        EditorWhiteBoxComponent::LayerMeta meta = c->GetLayerMeta(row);
                        meta.m_tint = AZ::Vector3(
                            aznumeric_cast<float>(picked.redF()), aznumeric_cast<float>(picked.greenF()),
                            aznumeric_cast<float>(picked.blueF()));
                        c->SetLayerMeta(row, meta);
                    });
            });

        // One handler applies every remaining meta control to the selected layer.
        const auto applyMeta = [this]()
        {
            if (m_updating)
            {
                return;
            }
            const int row = m_layerList->currentRow();
            if (row < 0)
            {
                return;
            }
            const auto combine = static_cast<LayerCombineMode>(m_layerCombineCombo->currentData().toInt());
            const bool invert = m_layerInvertNormals->isChecked();
            const bool edgesOnly = m_layerEdgesOnly->isChecked();
            const bool collision = m_layerCollision->isChecked();
            const AZ::Vector3 pos(
                aznumeric_cast<float>(m_layerPos[0]->value()), aznumeric_cast<float>(m_layerPos[1]->value()),
                aznumeric_cast<float>(m_layerPos[2]->value()));
            const AZ::Vector3 rot(
                aznumeric_cast<float>(m_layerRot[0]->value()), aznumeric_cast<float>(m_layerRot[1]->value()),
                aznumeric_cast<float>(m_layerRot[2]->value()));
            const AZ::Vector3 scale(
                aznumeric_cast<float>(m_layerScale[0]->value()), aznumeric_cast<float>(m_layerScale[1]->value()),
                aznumeric_cast<float>(m_layerScale[2]->value()));
            ModifyComponent(
                "White Box Layer Edit",
                [row, combine, invert, edgesOnly, collision, pos, rot, scale](EditorWhiteBoxComponent* c)
                {
                    EditorWhiteBoxComponent::LayerMeta meta = c->GetLayerMeta(row);
                    meta.m_combineMode = combine;
                    meta.m_invertNormals = invert;
                    meta.m_edgesOnly = edgesOnly;
                    meta.m_collision = collision;
                    meta.m_position = pos;
                    meta.m_rotation = rot;
                    meta.m_scale = scale;
                    c->SetLayerMeta(row, meta);
                });
        };
        connect(m_layerCombineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, applyMeta);
        connect(m_layerInvertNormals, &QCheckBox::toggled, this, applyMeta);
        connect(m_layerEdgesOnly, &QCheckBox::toggled, this, applyMeta);
        connect(m_layerCollision, &QCheckBox::toggled, this, applyMeta);
        for (QDoubleSpinBox* spin :
             { m_layerPos[0], m_layerPos[1], m_layerPos[2], m_layerRot[0], m_layerRot[1], m_layerRot[2],
               m_layerScale[0], m_layerScale[1], m_layerScale[2] })
        {
            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, applyMeta);
        }

        return group;
    }

    QWidget* WhiteBoxPaneWidget::BuildDrawSection()
    {
        auto* group = new QGroupBox(tr("Draw Shape"));
        auto* layout = new QFormLayout(group);

        m_drawShapeCombo = new QComboBox();
        m_drawShapeCombo->addItem(tr("Box"), static_cast<int>(DrawShapeType::Box));
        m_drawShapeCombo->addItem(tr("Cylinder"), static_cast<int>(DrawShapeType::Cylinder));
        m_drawShapeCombo->addItem(tr("Pyramid"), static_cast<int>(DrawShapeType::Pyramid));
        m_drawShapeCombo->addItem(tr("Cone"), static_cast<int>(DrawShapeType::Cone));
        m_drawShapeCombo->addItem(tr("Sphere"), static_cast<int>(DrawShapeType::Sphere));
        m_drawShapeCombo->addItem(tr("Staircase"), static_cast<int>(DrawShapeType::Staircase));
        m_drawShapeCombo->addItem(tr("Plane"), static_cast<int>(DrawShapeType::Plane));
        m_drawShapeCombo->addItem(tr("Torus"), static_cast<int>(DrawShapeType::Torus));
        m_drawShapeCombo->addItem(tr("Pipe"), static_cast<int>(DrawShapeType::Pipe));
        m_drawShapeCombo->addItem(tr("Freeform Polygon"), static_cast<int>(DrawShapeType::Polygon));
        layout->addRow(tr("Shape"), m_drawShapeCombo);
        auto* drawHint = new QLabel(tr(
            "Plane: drag and release. Solids: drag a base, then pull height and click. "
            "Polygon: click corners; Enter or click the first corner to finish; Backspace removes a corner. Esc cancels."));
        drawHint->setWordWrap(true);
        layout->addRow(drawHint);
        m_drawHoleRatio = MakeSpin(0.05, 0.95, 0.05);
        m_drawHoleRatioLabel = new QLabel(tr("Hole Ratio"));
        layout->addRow(m_drawHoleRatioLabel, m_drawHoleRatio);
        connect(m_drawHoleRatio, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double value)
            {
                if (!m_updating)
                {
                    ModifyComponent("White Box Hole Ratio",
                        [value](EditorWhiteBoxComponent* c) { c->SetDrawHoleRatio(static_cast<float>(value)); });
                }
            });

        m_drawTubeSides = new QSpinBox();
        m_drawTubeSides->setRange(MinTubeSides, MaxTubeSides);
        m_drawTubeSides->setToolTip(
            tr("Segments around the torus tube's cross-section. A torus has Sides x Tube Sides quads, so this "
               "is the other half of its triangle count."));
        m_drawTubeSidesLabel = new QLabel(tr("Tube Sides"));
        layout->addRow(m_drawTubeSidesLabel, m_drawTubeSides);
        connect(m_drawTubeSides, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int tubeSides)
            {
                if (!m_updating)
                {
                    ModifyComponent("White Box Tube Sides",
                        [tubeSides](EditorWhiteBoxComponent* c) { c->SetDrawTubeSides(tubeSides); });
                }
            });

        m_drawSides = new QSpinBox();
        m_drawSides->setRange(3, 128);
        m_drawSides->setToolTip(
            tr("Number of sides for round / N-gon shapes (4 = box / square), or the subdivision of the Sphere."));
        m_drawSidesLabel = new QLabel(tr("Sides"));
        layout->addRow(m_drawSidesLabel, m_drawSides);

        m_stairGroup = new QGroupBox(tr("Staircase"));
        auto* stairLayout = new QFormLayout(m_stairGroup);
        m_stairByHeight = new QCheckBox(tr("Divide By Step Height"));
        stairLayout->addRow(QString(), m_stairByHeight);
        m_stairSteps = new QSpinBox();
        m_stairSteps->setRange(1, 128);
        stairLayout->addRow(tr("Step Count"), m_stairSteps);
        m_stairStepHeight = MakeSpin(0.01, 1000.0, 0.05);
        stairLayout->addRow(tr("Step Height"), m_stairStepHeight);
        m_stairRotation = new QSpinBox();
        m_stairRotation->setRange(0, 3);
        m_stairRotation->setToolTip(tr("Orientation in 90-degree steps about the drawn surface."));
        stairLayout->addRow(tr("Rotation (x90)"), m_stairRotation);
        layout->addRow(m_stairGroup);

        m_drawCarve = new QCheckBox(tr("Carve (Boolean)"));
        m_drawCarve->setToolTip(
            tr("When on, drawing performs a CSG boolean (same as holding Ctrl): pull in to carve, out to add."));
        layout->addRow(QString(), m_drawCarve);
        m_drawMergeUnion = new QCheckBox(tr("Merge Draw Shape (Union)"));
        m_drawMergeUnion->setToolTip(
            tr("When on, committing a drawn shape CSG-unions it into the mesh instead of leaving overlapping "
               "geometry."));
        layout->addRow(QString(), m_drawMergeUnion);

        connect(
            m_drawShapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index)
            {
                if (m_updating)
                {
                    return;
                }
                const auto shape = static_cast<DrawShapeType>(m_drawShapeCombo->itemData(index).toInt());
                ModifyComponent(
                    "White Box Draw Shape",
                    [shape](EditorWhiteBoxComponent* c) { c->SetDrawShape(shape); });
            });
        connect(m_drawSides, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int sides)
            {
                if (!m_updating)
                {
                    ModifyComponent(
                        "White Box Draw Sides", [sides](EditorWhiteBoxComponent* c) { c->SetDrawSides(sides); });
                }
            });
        const auto applyStair = [this]()
        {
            if (m_updating)
            {
                return;
            }
            DrawStairInfo info;
            info.m_byHeight = m_stairByHeight->isChecked();
            info.m_steps = m_stairSteps->value();
            info.m_stepHeight = aznumeric_cast<float>(m_stairStepHeight->value());
            info.m_rotation = m_stairRotation->value();
            ModifyComponent(
                "White Box Stair Settings", [info](EditorWhiteBoxComponent* c) { c->SetDrawStairInfo(info); });
        };
        connect(m_stairByHeight, &QCheckBox::toggled, this, applyStair);
        connect(m_stairSteps, QOverload<int>::of(&QSpinBox::valueChanged), this, applyStair);
        connect(m_stairStepHeight, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, applyStair);
        connect(m_stairRotation, QOverload<int>::of(&QSpinBox::valueChanged), this, applyStair);
        connect(m_drawCarve, &QCheckBox::toggled, this,
            [this](bool carve)
            {
                if (!m_updating)
                {
                    ModifyComponent(
                        "White Box Draw Carve", [carve](EditorWhiteBoxComponent* c) { c->SetDrawCarve(carve); });
                }
            });
        connect(m_drawMergeUnion, &QCheckBox::toggled, this,
            [this](bool merge)
            {
                if (!m_updating)
                {
                    ModifyComponent(
                        "White Box Draw Merge",
                        [merge](EditorWhiteBoxComponent* c) { c->SetDrawMergeUnion(merge); });
                }
            });

        return group;
    }

    QWidget* WhiteBoxPaneWidget::BuildCubeStampSection()
    {
        auto* group = new QGroupBox(tr("Unit Cube Stamp"));
        auto* layout = new QFormLayout(group);

        m_unitCube = new QCheckBox(tr("Stamp Cubes In Draw Mode"));
        m_unitCube->setToolTip(
            tr("In draw mode, press to place a grid-snapped cube and drag to extrude it along the surface "
               "(hold Ctrl before drawing to subtract)."));
        layout->addRow(QString(), m_unitCube);
        m_unitCubeSize = MakeSpin(0.05, 100.0, 0.5, 2);
        m_unitCubeSize->setToolTip(tr("World-space size of the next stamped cube."));
        layout->addRow(tr("Cube Size"), m_unitCubeSize);
        m_unitCubeShowGrid = new QCheckBox(tr("Show Cube Grid Preview"));
        layout->addRow(QString(), m_unitCubeShowGrid);
        m_clearCubesButton = new QPushButton(tr("Clear Cube Stamp"));
        m_clearCubesButton->setToolTip(tr("Remove every cube placed with the Unit Cube Stamp tool."));
        layout->addRow(m_clearCubesButton);

        connect(m_unitCube, &QCheckBox::toggled, this,
            [this](bool on)
            {
                if (!m_updating)
                {
                    ModifyComponent(
                        "White Box Unit Cube", [on](EditorWhiteBoxComponent* c) { c->SetDrawUnitCube(on); });
                }
            });
        connect(m_unitCubeSize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double size)
            {
                if (!m_updating)
                {
                    ModifyComponent(
                        "White Box Cube Size",
                        [size](EditorWhiteBoxComponent* c)
                        { c->SetDrawUnitCubeSize(aznumeric_cast<float>(size)); });
                }
            });
        connect(m_unitCubeShowGrid, &QCheckBox::toggled, this,
            [this](bool show)
            {
                if (!m_updating)
                {
                    ModifyComponent(
                        "White Box Cube Grid Preview",
                        [show](EditorWhiteBoxComponent* c) { c->SetDrawUnitCubeShowGrid(show); });
                }
            });
        connect(m_clearCubesButton, &QPushButton::clicked, this,
            [this]() {
                ModifyComponent("White Box Clear Cubes", [](EditorWhiteBoxComponent* c) { c->ClearCubeStamp(); });
            });

        return group;
    }

    QWidget* WhiteBoxPaneWidget::BuildBooleanSection()
    {
        auto* group = new QGroupBox(tr("Boolean"));
        auto* layout = new QFormLayout(group);

        m_csgSolverCombo = new QComboBox();
        m_csgSolverCombo->setToolTip(tr(
            "CSG solver used for EVERY boolean on this component - the per-layer Combine modes and the "
            "entity Boolean below.\n"
            "Fast (BSP): brush-style, face-based. Handles inward-facing shells (rooms/corridors) and "
            "inverted-normal meshes, and tolerates open/non-manifold geometry. Best for blocky level "
            "geometry.\n"
            "Manifold: volumetric and precise, but needs closed watertight meshes and treats inverted "
            "normals as a shape's complement."));
        m_csgSolverCombo->addItem(tr("Manifold"), static_cast<int>(Api::CsgSolver::Manifold));
        m_csgSolverCombo->addItem(tr("Fast (BSP)"), static_cast<int>(Api::CsgSolver::Fast));
        layout->addRow(tr("Solver"), m_csgSolverCombo);

        m_booleanSourceCombo = new QComboBox();
        m_booleanSourceCombo->setToolTip(
            tr("Another entity with a White Box component to use as the boolean operand."));
        layout->addRow(tr("Source"), m_booleanSourceCombo);
        m_booleanOpCombo = new QComboBox();
        m_booleanOpCombo->addItem(tr("Subtract"), static_cast<int>(Api::BooleanOperation::Subtraction));
        m_booleanOpCombo->addItem(tr("Union"), static_cast<int>(Api::BooleanOperation::Union));
        m_booleanOpCombo->addItem(tr("Intersect"), static_cast<int>(Api::BooleanOperation::Intersection));
        layout->addRow(tr("Operation"), m_booleanOpCombo);
        m_booleanLive = new QCheckBox(tr("Non-Destructive (Live)"));
        layout->addRow(QString(), m_booleanLive);
        m_booleanActiveOnly = new QCheckBox(tr("Affect Only The Active Layer"));
        layout->addRow(QString(), m_booleanActiveOnly);
        m_booleanSourceAfterCombo = new QComboBox();
        m_booleanSourceAfterCombo->setToolTip(tr("What happens to the source entity after Apply Boolean."));
        m_booleanSourceAfterCombo->addItem(tr("Keep Source"), static_cast<int>(SourceAfterApply::Keep));
        m_booleanSourceAfterCombo->addItem(tr("Hide Source"), static_cast<int>(SourceAfterApply::Hide));
        m_booleanSourceAfterCombo->addItem(tr("Delete Source"), static_cast<int>(SourceAfterApply::Delete));
        layout->addRow(tr("After Apply"), m_booleanSourceAfterCombo);
        m_applyBooleanButton = new QPushButton(tr("Apply Boolean"));
        layout->addRow(m_applyBooleanButton);

        // --- Global (scene-wide) boolean -------------------------------------------------------
        auto* globalLabel = new QLabel(tr("<b>Global Boolean</b>"));
        layout->addRow(globalLabel);
        m_booleanOthers = new QCheckBox(tr("Boolean Other Entities (Cutter)"));
        m_booleanOthers->setToolTip(
            tr("When on, this entity acts as a boolean brush: it cuts every overlapping White Box entity "
               "that is not excluded. Use Refresh Global Booleans to apply."));
        layout->addRow(QString(), m_booleanOthers);
        m_cutterOpCombo = new QComboBox();
        m_cutterOpCombo->setToolTip(tr("Operation this cutter applies to the entities it overlaps."));
        m_cutterOpCombo->addItem(tr("Subtract"), static_cast<int>(Api::BooleanOperation::Subtraction));
        m_cutterOpCombo->addItem(tr("Union"), static_cast<int>(Api::BooleanOperation::Union));
        m_cutterOpCombo->addItem(tr("Intersect"), static_cast<int>(Api::BooleanOperation::Intersection));
        layout->addRow(tr("Cutter Operation"), m_cutterOpCombo);
        m_excludeFromBoolean = new QCheckBox(tr("Exclude From Boolean"));
        m_excludeFromBoolean->setToolTip(tr("When on, this entity is never cut by global cutters."));
        layout->addRow(QString(), m_excludeFromBoolean);
        m_refreshGlobalBooleansButton = new QPushButton(tr("Refresh Global Booleans"));
        m_refreshGlobalBooleansButton->setToolTip(
            tr("Re-evaluate every non-excluded White Box entity against the current set of cutters."));
        layout->addRow(m_refreshGlobalBooleansButton);

        connect(
            m_csgSolverCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index)
            {
                if (m_updating || index < 0)
                {
                    return;
                }
                const auto solver = static_cast<Api::CsgSolver>(m_csgSolverCombo->itemData(index).toInt());
                ModifyComponent(
                    "White Box CSG Solver", [solver](EditorWhiteBoxComponent* c) { c->SetCsgSolver(solver); });
            });
        connect(
            m_booleanSourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index)
            {
                if (m_updating || index < 0)
                {
                    return;
                }
                const AZ::EntityId sourceId(m_booleanSourceCombo->itemData(index).toULongLong());
                ModifyComponent(
                    "White Box Boolean Source",
                    [sourceId](EditorWhiteBoxComponent* c) { c->SetBooleanSourceEntity(sourceId); });
            });
        connect(
            m_booleanOpCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index)
            {
                if (m_updating)
                {
                    return;
                }
                const auto op = static_cast<Api::BooleanOperation>(m_booleanOpCombo->itemData(index).toInt());
                ModifyComponent(
                    "White Box Boolean Operation",
                    [op](EditorWhiteBoxComponent* c) { c->SetBooleanOperation(op); });
            });
        connect(m_booleanLive, &QCheckBox::toggled, this,
            [this](bool live)
            {
                if (!m_updating)
                {
                    ModifyComponent(
                        "White Box Live Boolean", [live](EditorWhiteBoxComponent* c) { c->SetLiveBoolean(live); });
                }
            });
        connect(m_booleanActiveOnly, &QCheckBox::toggled, this,
            [this](bool activeOnly)
            {
                if (!m_updating)
                {
                    ModifyComponent(
                        "White Box Boolean Target",
                        [activeOnly](EditorWhiteBoxComponent* c) { c->SetBooleanAffectActiveOnly(activeOnly); });
                }
            });
        connect(
            m_booleanSourceAfterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index)
            {
                if (m_updating || index < 0)
                {
                    return;
                }
                const auto mode = static_cast<SourceAfterApply>(m_booleanSourceAfterCombo->itemData(index).toInt());
                ModifyComponent(
                    "White Box Boolean Source Fate",
                    [mode](EditorWhiteBoxComponent* c) { c->SetSourceAfterApply(mode); });
            });
        connect(m_applyBooleanButton, &QPushButton::clicked, this,
            [this]() {
                ModifyComponent("White Box Apply Boolean", [](EditorWhiteBoxComponent* c) { c->ApplyBoolean(); });
            });

        connect(m_booleanOthers, &QCheckBox::toggled, this,
            [this](bool booleanOthers)
            {
                if (!m_updating)
                {
                    ModifyComponent(
                        "White Box Boolean Others",
                        [booleanOthers](EditorWhiteBoxComponent* c) { c->SetBooleanOthers(booleanOthers); });
                }
            });
        connect(
            m_cutterOpCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index)
            {
                if (m_updating || index < 0)
                {
                    return;
                }
                const auto op = static_cast<Api::BooleanOperation>(m_cutterOpCombo->itemData(index).toInt());
                ModifyComponent(
                    "White Box Cutter Operation",
                    [op](EditorWhiteBoxComponent* c) { c->SetCutterOperation(op); });
            });
        connect(m_excludeFromBoolean, &QCheckBox::toggled, this,
            [this](bool exclude)
            {
                if (!m_updating)
                {
                    ModifyComponent(
                        "White Box Exclude From Boolean",
                        [exclude](EditorWhiteBoxComponent* c) { c->SetExcludeFromBoolean(exclude); });
                }
            });
        connect(m_refreshGlobalBooleansButton, &QPushButton::clicked, this,
            []()
            {
                // Scene-wide re-evaluate. Wrap in one undo batch so the whole refresh is a single
                // undo step (RefreshGlobalBooleans touches many entities).
                AzToolsFramework::ScopedUndoBatch undoBatch("White Box Refresh Global Booleans");
                EditorWhiteBoxComponent::RefreshGlobalBooleans();
            });

        return group;
    }

    QWidget* WhiteBoxPaneWidget::BuildVertexPaintSection()
    {
        auto* group = new QGroupBox(tr("Vertex Paint"));
        auto* layout = new QFormLayout(group);
        auto* hint = new QLabel(tr(
            "Choose Vertex Paint above, then click or drag over triangle faces of the active layer. "
            "Escape or right-click cancels a stroke. Painting makes parametric layers editable meshes."));
        hint->setWordWrap(true);
        layout->addRow(hint);
        m_paintOperation = new QComboBox(group);
        m_paintOperation->addItem(tr("Paint Material"), static_cast<int>(FacePaintOperation::Material));
        m_paintOperation->addItem(tr("Paint Color"), static_cast<int>(FacePaintOperation::Color));
        m_paintOperation->addItem(tr("Reset Material"), static_cast<int>(FacePaintOperation::ResetMaterial));
        m_paintOperation->addItem(tr("Reset Color"), static_cast<int>(FacePaintOperation::ResetColor));
        layout->addRow(tr("Operation"), m_paintOperation);
        m_paintMaterial = new AzToolsFramework::PropertyAssetCtrl(group);
        m_paintMaterial->SetCurrentAssetType(azrtti_typeid<AZ::RPI::MaterialAsset>());
        layout->addRow(tr("Material"), m_paintMaterial);
        m_paintColor = new QPushButton(tr("Choose Color"), group);
        m_paintColor->setToolTip(tr(
            "Paint an opaque face color. Custom materials need a base color property; their textures remain enabled."));
        layout->addRow(tr("Color"), m_paintColor);
        connect(m_paintOperation, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int)
            {
                if (auto* component = CurrentComponent(); component && !m_updating)
                {
                    auto settings = component->GetFacePaintSettings();
                    settings.m_operation = static_cast<FacePaintOperation>(m_paintOperation->currentData().toInt());
                    component->SetFacePaintSettings(settings);
                    RefreshFromComponent();
                }
            });
        connect(m_paintMaterial, &AzToolsFramework::PropertyAssetCtrl::OnAssetIDChanged, this,
            [this](const AZ::Data::AssetId& material)
            {
                if (auto* component = CurrentComponent(); component && !m_updating)
                {
                    auto settings = component->GetFacePaintSettings();
                    settings.m_material = material;
                    component->SetFacePaintSettings(settings);
                }
            });
        connect(m_paintColor, &QPushButton::clicked, this,
            [this]()
            {
                auto* component = CurrentComponent();
                if (!component)
                {
                    return;
                }
                auto settings = component->GetFacePaintSettings();
                const QColor initial(settings.m_color & 255, (settings.m_color >> 8) & 255, (settings.m_color >> 16) & 255);
                const QColor chosen = QColorDialog::getColor(initial, this, tr("Face Paint Color"));
                if (chosen.isValid())
                {
                    settings.m_color = static_cast<AZ::u32>(chosen.red()) |
                        (static_cast<AZ::u32>(chosen.green()) << 8) |
                        (static_cast<AZ::u32>(chosen.blue()) << 16) | 0xFF000000u;
                    component->SetFacePaintSettings(settings);
                    RefreshFromComponent();
                }
            });
        return group;
    }

    QWidget* WhiteBoxPaneWidget::BuildMaterialSection()
    {
        auto* group = new QGroupBox(tr("Material / Display"));
        auto* layout = new QFormLayout(group);

        m_defaultMaterial = new AzToolsFramework::PropertyAssetCtrl(group);
        m_defaultMaterial->SetCurrentAssetType(azrtti_typeid<AZ::RPI::MaterialAsset>());
        m_defaultMaterial->setToolTip(tr("Default material for this entity. Clear to use the built-in WhiteBox material."));
        layout->addRow(tr("Default Material"), m_defaultMaterial);
        connect(m_defaultMaterial, &AzToolsFramework::PropertyAssetCtrl::OnAssetIDChanged, this,
            [this](const AZ::Data::AssetId& material)
            {
                if (!m_updating)
                {
                    ModifyComponent("White Box Default Material",
                        [material](EditorWhiteBoxComponent* c) { c->SetMaterialOverride(material); });
                }
            });

        auto* selectionHint = new QLabel(tr("In Transform mode, Ctrl-click polygons to add or remove them from the selection."), group);
        selectionHint->setWordWrap(true);
        layout->addRow(selectionHint);
        m_polygonSelectionLabel = new QLabel(group);
        layout->addRow(m_polygonSelectionLabel);
        m_polygonMaterial = new AzToolsFramework::PropertyAssetCtrl(group);
        m_polygonMaterial->SetCurrentAssetType(azrtti_typeid<AZ::RPI::MaterialAsset>());
        layout->addRow(tr("Polygon Material"), m_polygonMaterial);
        auto* materialButtons = new QHBoxLayout();
        m_assignPolygonMaterial = new QPushButton(tr("Assign Material"), group);
        m_resetPolygonMaterial = new QPushButton(tr("Reset to Default"), group);
        m_assignPolygonMaterial->setToolTip(tr("Assign to selected polygons. Parametric layers become editable meshes."));
        materialButtons->addWidget(m_assignPolygonMaterial);
        materialButtons->addWidget(m_resetPolygonMaterial);
        layout->addRow(materialButtons);
        connect(m_assignPolygonMaterial, &QPushButton::clicked, this,
            [this]() { AssignSelectedPolygonMaterial(false); });
        connect(m_resetPolygonMaterial, &QPushButton::clicked, this,
            [this]() { AssignSelectedPolygonMaterial(true); });
        connect(m_polygonMaterial, &AzToolsFramework::PropertyAssetCtrl::OnAssetIDChanged, this,
            [this](const AZ::Data::AssetId&) { RefreshPolygonMaterialSelection(); });
        // Selection changes do not modify the component. Keep only these lightweight controls in sync.
        auto* selectionTimer = new QTimer(this);
        connect(selectionTimer, &QTimer::timeout, this, &WhiteBoxPaneWidget::RefreshPolygonMaterialSelection);
        selectionTimer->start(150);
        RefreshPolygonMaterialSelection();

        m_useGlobalTint = new QCheckBox(tr("Use Global Tint"));
        m_useGlobalTint->setToolTip(
            tr("When on, every layer renders with the global tint below; when off each layer uses its own tint."));
        layout->addRow(QString(), m_useGlobalTint);
        m_globalTintButton = new QPushButton();
        m_globalTintButton->setToolTip(tr("Tint for the built-in WhiteBox material. Custom materials keep their authored settings."));
        layout->addRow(tr("Tint"), m_globalTintButton);
        m_useTexture = new QCheckBox(tr("Use Texture"));
        m_useTexture->setToolTip(tr("Texture toggle for the built-in WhiteBox material."));
        layout->addRow(QString(), m_useTexture);
        m_edgesOnly = new QCheckBox(tr("Edges Only"));
        m_edgesOnly->setToolTip(tr("Hide the solid render mesh and draw only the mesh edges."));
        layout->addRow(QString(), m_edgesOnly);

        connect(m_useGlobalTint, &QCheckBox::toggled, this,
            [this](bool use)
            {
                if (!m_updating)
                {
                    ModifyComponent(
                        "White Box Global Tint", [use](EditorWhiteBoxComponent* c) { c->SetUseGlobalTint(use); });
                }
            });
        connect(m_globalTintButton, &QPushButton::clicked, this,
            [this]()
            {
                EditorWhiteBoxComponent* component = CurrentComponent();
                if (component == nullptr)
                {
                    return;
                }
                const AZ::Color current = component->GetMaterialTint();
                const QColor picked = QColorDialog::getColor(
                    QColor::fromRgbF(current.GetR(), current.GetG(), current.GetB()), this, tr("White Box Tint"));
                if (!picked.isValid())
                {
                    return;
                }
                ModifyComponent(
                    "White Box Tint",
                    [picked](EditorWhiteBoxComponent* c)
                    {
                        c->SetMaterialTint(AZ::Color(
                            aznumeric_cast<float>(picked.redF()), aznumeric_cast<float>(picked.greenF()),
                            aznumeric_cast<float>(picked.blueF()), 1.0f));
                    });
            });
        connect(m_useTexture, &QCheckBox::toggled, this,
            [this](bool use)
            {
                if (!m_updating)
                {
                    ModifyComponent(
                        "White Box Texture",
                        [use](EditorWhiteBoxComponent* c) { c->SetMaterialUseTexture(use); });
                }
            });
        connect(m_edgesOnly, &QCheckBox::toggled, this,
            [this](bool edgesOnly)
            {
                if (!m_updating)
                {
                    ModifyComponent(
                        "White Box Edges Only",
                        [edgesOnly](EditorWhiteBoxComponent* c) { c->SetEdgesOnly(edgesOnly); });
                }
            });

        return group;
    }

    void WhiteBoxPaneWidget::RefreshPolygonMaterialSelection()
    {
        if (!m_polygonSelectionLabel)
        {
            return;
        }
        Api::PolygonHandles polygons;
        if (auto* component = CurrentComponent())
        {
            EditorWhiteBoxTransformModeRequestBus::EventResult(
                polygons, AZ::EntityComponentIdPair(m_currentEntityId, component->GetId()),
                &EditorWhiteBoxTransformModeRequests::GetSelectedPolygons);
        }
        m_polygonSelectionLabel->setText(tr("%1 polygons selected").arg(static_cast<int>(polygons.size())));
        m_assignPolygonMaterial->setEnabled(!polygons.empty() && m_polygonMaterial->GetSelectedAssetID().IsValid());
        m_resetPolygonMaterial->setEnabled(!polygons.empty());
    }

    void WhiteBoxPaneWidget::AssignSelectedPolygonMaterial(const bool reset)
    {
        auto* component = CurrentComponent();
        if (!component)
        {
            return;
        }
        Api::PolygonHandles polygons;
        EditorWhiteBoxTransformModeRequestBus::EventResult(
            polygons, AZ::EntityComponentIdPair(m_currentEntityId, component->GetId()),
            &EditorWhiteBoxTransformModeRequests::GetSelectedPolygons);
        const AZ::Data::AssetId material = reset ? AZ::Data::AssetId{} : m_polygonMaterial->GetSelectedAssetID();
        if (polygons.empty() || (!reset && !material.IsValid()))
        {
            return;
        }
        ModifyComponent(reset ? "White Box Reset Polygon Material" : "White Box Assign Polygon Material",
            [polygons, material](EditorWhiteBoxComponent* c) { c->AssignPolygonMaterial(polygons, material); });
    }

    QWidget* WhiteBoxPaneWidget::BuildMeshOpsSection()
    {
        auto* group = new QGroupBox(tr("Mesh / Asset"));
        auto* layout = new QVBoxLayout(group);

        auto* opsRow = new QHBoxLayout();
        auto* fixButton = new QPushButton(tr("Fix Non-Manifold"));
        fixButton->setToolTip(
            tr("Weld coincident vertices and regroup coplanar faces so the whole mesh is a clean manifold."));
        auto* collisionButton = new QPushButton(tr("Add Collision"));
        collisionButton->setToolTip(tr("Add a White Box collider component to this entity."));
        opsRow->addWidget(fixButton);
        opsRow->addWidget(collisionButton);
        layout->addLayout(opsRow);

        auto* assetRow = new QHBoxLayout();
        auto* saveAsButton = new QPushButton(tr("Save As Asset..."));
        auto* exportButton = new QPushButton(tr("Export"));
        exportButton->setToolTip(tr("Export to obj."));
        auto* exportDescendantsButton = new QPushButton(tr("Export Descendants"));
        exportDescendantsButton->setToolTip(
            tr("Export all whiteboxes on descendant entities as a single obj (excluding this one)."));
        assetRow->addWidget(saveAsButton);
        assetRow->addWidget(exportButton);
        assetRow->addWidget(exportDescendantsButton);
        layout->addLayout(assetRow);

        m_flipYZ = new QCheckBox(tr("Flip Y and Z For Export"));
        layout->addWidget(m_flipYZ);

        connect(fixButton, &QPushButton::clicked, this,
            [this]() {
                ModifyComponent("White Box Fix Mesh", [](EditorWhiteBoxComponent* c) { c->FixNonManifold(); });
            });
        connect(collisionButton, &QPushButton::clicked, this,
            [this]() {
                ModifyComponent("White Box Add Collision", [](EditorWhiteBoxComponent* c) { c->AddCollision(); });
            });
        connect(saveAsButton, &QPushButton::clicked, this,
            [this]()
            {
                if (EditorWhiteBoxComponent* component = CurrentComponent())
                {
                    component->SaveMeshAsAsset(); // runs its own dialog + undo handling
                    RefreshFromComponent();
                }
            });
        connect(exportButton, &QPushButton::clicked, this,
            [this]()
            {
                if (EditorWhiteBoxComponent* component = CurrentComponent())
                {
                    component->ExportToFile();
                }
            });
        connect(exportDescendantsButton, &QPushButton::clicked, this,
            [this]()
            {
                if (EditorWhiteBoxComponent* component = CurrentComponent())
                {
                    component->ExportDescendantsToFile();
                }
            });
        connect(m_flipYZ, &QCheckBox::toggled, this,
            [this](bool flip)
            {
                if (!m_updating)
                {
                    ModifyComponent(
                        "White Box Export Axes",
                        [flip](EditorWhiteBoxComponent* c) { c->SetFlipYZForExport(flip); });
                }
            });

        return group;
    }

    // ---- data plumbing --------------------------------------------------------------------

    EditorWhiteBoxComponent* WhiteBoxPaneWidget::CurrentComponent() const
    {
        if (AZ::Entity* entity = FindEntity(m_currentEntityId))
        {
            return entity->FindComponent<EditorWhiteBoxComponent>();
        }
        return nullptr;
    }

    void WhiteBoxPaneWidget::ModifyComponent(
        const char* undoLabel, const AZStd::function<void(EditorWhiteBoxComponent*)>& fn)
    {
        EditorWhiteBoxComponent* component = CurrentComponent();
        if (component == nullptr)
        {
            return;
        }
        {
            AzToolsFramework::ScopedUndoBatch undoBatch(undoLabel);
            fn(component);
            undoBatch.MarkEntityDirty(m_currentEntityId);
        }
        RefreshFromComponent();
    }

    void WhiteBoxPaneWidget::MoveLayerRow(const int from, const int to)
    {
        EditorWhiteBoxComponent* component = CurrentComponent();
        if (component == nullptr)
        {
            return;
        }
        const int layerCount = component->GetLayerCount();
        if (from < 0 || from >= layerCount || to < 0 || to >= layerCount || from == to)
        {
            return; // nothing selected, or already at the top / bottom
        }

        ModifyComponent(
            "White Box Reorder Layer",
            [from, to](EditorWhiteBoxComponent* c)
            {
                c->MoveLayer(from, to);
                // Selecting a layer in this list is what makes it the edit target, so keep the
                // two in step: the moved layer stays selected below, so make it active too.
                c->SetActiveLayer(to);
            });

        // RefreshLayerControls restores the previously selected ROW, which now holds a different
        // layer - follow the layer the user moved instead.
        const bool wasUpdating = m_updating;
        m_updating = true;
        m_layerList->setCurrentRow(to);
        m_updating = wasUpdating;
        RefreshFromComponent();
    }

    void WhiteBoxPaneWidget::RefreshEntityList()
    {
        const bool wasUpdating = m_updating;
        m_updating = true;

        const AZStd::vector<AZ::EntityId> entityIds = CollectWhiteBoxEntities();

        m_entityCombo->clear();
        int currentIndex = -1;
        for (const AZ::EntityId& entityId : entityIds)
        {
            m_entityCombo->addItem(EntityDisplayName(entityId), QVariant(static_cast<qulonglong>(static_cast<AZ::u64>(entityId))));
            if (entityId == m_currentEntityId)
            {
                currentIndex = m_entityCombo->count() - 1;
            }
        }

        if (currentIndex >= 0)
        {
            m_entityCombo->setCurrentIndex(currentIndex);
        }
        else if (m_entityCombo->count() > 0)
        {
            m_entityCombo->setCurrentIndex(0);
            m_currentEntityId = AZ::EntityId(m_entityCombo->itemData(0).toULongLong());
        }
        else
        {
            m_currentEntityId = AZ::EntityId();
        }

        // Boolean source: None + every White Box entity except the current one.
        m_booleanSourceCombo->clear();
        m_booleanSourceCombo->addItem(
            tr("None"), QVariant(static_cast<qulonglong>(static_cast<AZ::u64>(AZ::EntityId()))));
        for (const AZ::EntityId& entityId : entityIds)
        {
            if (entityId != m_currentEntityId)
            {
                m_booleanSourceCombo->addItem(
                    EntityDisplayName(entityId), QVariant(static_cast<qulonglong>(static_cast<AZ::u64>(entityId))));
            }
        }

        m_updating = wasUpdating;
    }

    void WhiteBoxPaneWidget::RefreshLayerControls(EditorWhiteBoxComponent* component)
    {
        // Active layer combo.
        m_activeLayerCombo->clear();
        const auto layerNames = component->GetLayerNames();
        for (const auto& [index, name] : layerNames)
        {
            m_activeLayerCombo->addItem(QString::fromUtf8(name.c_str()), index);
        }
        const int activeIndex = component->GetActiveLayerIndex();
        for (int i = 0; i < m_activeLayerCombo->count(); ++i)
        {
            if (m_activeLayerCombo->itemData(i).toInt() == activeIndex)
            {
                m_activeLayerCombo->setCurrentIndex(i);
                break;
            }
        }

        // Layer list (preserve the selected row across the rebuild).
        const int previousRow = m_layerList->currentRow();
        m_layerList->clear();
        const int layerCount = component->GetLayerCount();
        for (int i = 0; i < layerCount; ++i)
        {
            const EditorWhiteBoxComponent::LayerMeta meta = component->GetLayerMeta(i);
            auto* item = new QListWidgetItem(QString::fromUtf8(meta.m_name.c_str()));
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
            item->setCheckState(meta.m_visible ? Qt::Checked : Qt::Unchecked);
            m_layerList->addItem(item);
        }
        const int row = (previousRow >= 0 && previousRow < layerCount) ? previousRow
            : (layerCount > 0 ? AZStd::clamp(activeIndex, 0, layerCount - 1) : -1);
        m_layerList->setCurrentRow(row);

        // Size the list to its contents so every layer is visible without an inner scrollbar.
        const int rowHeight = AZStd::max(m_layerList->sizeHintForRow(0), 18);
        m_layerList->setFixedHeight(
            AZStd::max(layerCount, 1) * rowHeight + 2 * m_layerList->frameWidth() + 4);

        // Meta editors for the selected layer.
        const bool hasLayer = row >= 0;
        m_layerMetaGroup->setEnabled(hasLayer);

        // Shape Parameters only exist for parametric layers.
        const bool parametric = hasLayer && component->IsLayerParametric(row);
        m_shapeParamsGroup->setVisible(parametric);
        if (parametric)
        {
            const EditorWhiteBoxComponent::ShapeParams params = component->GetLayerShapeParams(row);
            m_shapeParamShape->setCurrentIndex(m_shapeParamShape->findData(static_cast<int>(params.m_shape)));
            m_shapeParamWidth->setValue(params.m_width);
            m_shapeParamDepth->setValue(params.m_depth);
            m_shapeParamHeight->setValue(params.m_height);
            m_shapeParamSides->setValue(params.m_sides);
            m_shapeParamSteps->setValue(params.m_steps);
            m_shapeParamStepsByHeight->setChecked(params.m_stepsByHeight);
            m_shapeParamStepHeight->setValue(params.m_stepHeight);
            m_shapeParamWallThickness->setValue(params.m_wallThickness);
            m_shapeParamCavityGap->setValue(params.m_cavityGap);
            m_shapeParamFloor->setChecked(params.m_floor);
            m_shapeParamCeiling->setChecked(params.m_ceiling);
            m_shapeParamDoorFrame->setChecked(params.m_doorFrame);
            m_shapeParamArchHeight->setMaximum(AZStd::max(0.0f, params.m_height - 0.001f));
            m_shapeParamArchHeight->setValue(params.m_archHeight);
            m_shapeParamInnerRadius->setValue(params.m_innerRadius);
            m_shapeParamSweepAngle->setValue(params.m_sweepAngle);
            m_shapeParamHoleRatio->setValue(params.m_holeRatio);
            m_shapeParamTubeSides->setValue(params.m_tubeSides);
            // Room has no Sides/Steps; every other non-staircase shape honours Sides (round shapes
            // inscribe the N-gon in the ellipse, angular shapes fill the rectangle with it).
            const bool circular = params.m_shape == DrawShapeType::CircularStairs;
            const bool door = params.m_shape == DrawShapeType::Door;
            const bool stair = params.m_shape == DrawShapeType::Staircase || circular;
            const bool room = params.m_shape == DrawShapeType::Room;
            const bool plane = params.m_shape == DrawShapeType::Plane;
            const bool torus = params.m_shape == DrawShapeType::Torus;
            const bool ringShape = params.m_shape == DrawShapeType::Pipe || torus;
            const bool hasSides = !plane && !stair && !room && (!door || params.m_archHeight > 0.0f);
            m_shapeParamSides->setVisible(hasSides);
            m_shapeParamSidesLabel->setVisible(hasSides);
            m_shapeParamSidesLabel->setText(
                door ? tr("Arch Segments") : (params.m_shape == DrawShapeType::Sphere ? tr("Subdivision") : tr("Sides")));
            m_shapeParamTubeSides->setVisible(torus);
            m_shapeParamTubeSidesLabel->setVisible(torus);
            m_shapeParamSteps->setVisible(stair && !params.m_stepsByHeight);
            m_shapeParamStepsLabel->setVisible(stair && !params.m_stepsByHeight);
            m_shapeParamStepsByHeight->setVisible(stair);
            m_shapeParamStepHeight->setVisible(stair && params.m_stepsByHeight);
            m_shapeParamStepHeightLabel->setVisible(stair && params.m_stepsByHeight);
            // Room-only controls (Width/Depth/Height read as the interior dimensions for a Room).
            m_shapeParamWallThickness->setVisible(room || (door && params.m_doorFrame));
            m_shapeParamWallThicknessLabel->setVisible(room || (door && params.m_doorFrame));
            m_shapeParamWallThicknessLabel->setText(door ? tr("Frame Thickness") : tr("Wall Thickness"));
            m_shapeParamWallThickness->setToolTip(door ? tr("Thickness around the door opening.") : tr("Thickness of each wall leaf."));
            m_shapeParamCavityGap->setVisible(room);
            m_shapeParamCavityGapLabel->setVisible(room);
            m_shapeParamFloor->setVisible(room);
            m_shapeParamCeiling->setVisible(room);
            m_shapeParamDoorFrame->setVisible(door);
            auto* shapeLayout = qobject_cast<QFormLayout*>(m_shapeParamsGroup->layout());
            const auto showField = [shapeLayout](QWidget* field, bool visible)
            {
                field->setVisible(visible);
                if (auto* label = shapeLayout->labelForField(field))
                {
                    label->setVisible(visible);
                }
            };
            showField(m_shapeParamArchHeight, door);
            showField(m_shapeParamInnerRadius, circular);
            showField(m_shapeParamSweepAngle, circular);
            showField(m_shapeParamDepth, !circular);
            showField(m_shapeParamHeight, !plane);
            showField(m_shapeParamHoleRatio, ringShape);
            const auto setLabel = [shapeLayout](QWidget* field, const QString& text)
            {
                if (auto* label = qobject_cast<QLabel*>(shapeLayout->labelForField(field)))
                {
                    label->setText(text);
                }
            };
            setLabel(m_shapeParamWidth, circular ? tr("Tread Width") :
                (door && params.m_doorFrame ? tr("Opening Width") : tr("Width")));
            setLabel(m_shapeParamHeight, door && params.m_doorFrame ? tr("Opening Height") : tr("Height"));
            setLabel(m_shapeParamDepth, door ? tr("Thickness / Depth") : tr("Depth"));
        }

        if (hasLayer)
        {
            const EditorWhiteBoxComponent::LayerMeta meta = component->GetLayerMeta(row);
            SetColorButtonSwatch(m_layerTintButton, ToQColor(meta.m_tint));
            m_layerCombineCombo->setCurrentIndex(
                m_layerCombineCombo->findData(static_cast<int>(meta.m_combineMode)));
            m_layerInvertNormals->setChecked(meta.m_invertNormals);
            m_layerEdgesOnly->setChecked(meta.m_edgesOnly);
            m_layerCollision->setChecked(meta.m_collision);
            const AZ::Vector3 vecs[3] = { meta.m_position, meta.m_rotation, meta.m_scale };
            QDoubleSpinBox* (*rows[3])[3] = { &m_layerPos, &m_layerRot, &m_layerScale };
            for (int v = 0; v < 3; ++v)
            {
                (*rows[v])[0]->setValue(vecs[v].GetX());
                (*rows[v])[1]->setValue(vecs[v].GetY());
                (*rows[v])[2]->setValue(vecs[v].GetZ());
            }
        }
    }

    void WhiteBoxPaneWidget::UpdateEntityTransformUi()
    {
        if (!m_currentEntityId.IsValid())
        {
            return;
        }
        const bool wasUpdating = m_updating;
        m_updating = true;

        const bool world = m_entitySpaceCombo->currentIndex() == 1;
        AZ::Vector3 translation = AZ::Vector3::CreateZero();
        AZ::Quaternion rotation = AZ::Quaternion::CreateIdentity();
        if (world)
        {
            AZ::TransformBus::EventResult(
                translation, m_currentEntityId, &AZ::TransformBus::Events::GetWorldTranslation);
            AZ::TransformBus::EventResult(
                rotation, m_currentEntityId, &AZ::TransformBus::Events::GetWorldRotationQuaternion);
        }
        else
        {
            AZ::TransformBus::EventResult(
                translation, m_currentEntityId, &AZ::TransformBus::Events::GetLocalTranslation);
            AZ::TransformBus::EventResult(
                rotation, m_currentEntityId, &AZ::TransformBus::Events::GetLocalRotationQuaternion);
        }
        float scale = 1.0f;
        AZ::TransformBus::EventResult(scale, m_currentEntityId, &AZ::TransformBus::Events::GetLocalUniformScale);

        const AZ::Vector3 rotationDegrees = rotation.GetEulerDegrees();
        for (int i = 0; i < 3; ++i)
        {
            m_entityPos[i]->setValue(translation.GetElement(i));
            m_entityRot[i]->setValue(rotationDegrees.GetElement(i));
        }
        m_entityScale->setValue(scale);

        // Non-uniform scale from the entity's Non-Uniform Scale component (identity when absent).
        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(
            nonUniformScale, m_currentEntityId, &AZ::NonUniformScaleRequests::GetScale);
        for (int i = 0; i < 3; ++i)
        {
            m_entityNonUniformScale[i]->setValue(nonUniformScale.GetElement(i));
        }

        m_updating = wasUpdating;
    }

    void WhiteBoxPaneWidget::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
        UpdateEntityTransformUi();
    }

    void WhiteBoxPaneWidget::RefreshFromComponent()
    {
        m_updating = true;

        EditorWhiteBoxComponent* component = CurrentComponent();
        const bool hasComponent = component != nullptr;

        // Follow the current entity's transform (for the Entity Transform section).
        if (m_transformBusEntityId != m_currentEntityId)
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect();
            if (m_currentEntityId.IsValid())
            {
                AZ::TransformNotificationBus::Handler::BusConnect(m_currentEntityId);
            }
            m_transformBusEntityId = m_currentEntityId;
        }
        UpdateEntityTransformUi();

        // Follow the current component's white box notifications (viewport edits - drawing,
        // stamping - change component state the pane must mirror, e.g. the auto-created first layer).
        const AZ::EntityComponentIdPair meshBusPair =
            hasComponent ? AZ::EntityComponentIdPair(m_currentEntityId, component->GetId()) : AZ::EntityComponentIdPair();
        if (!(meshBusPair == m_meshBusPair))
        {
            EditorWhiteBoxComponentNotificationBus::Handler::BusDisconnect();
            if (hasComponent)
            {
                EditorWhiteBoxComponentNotificationBus::Handler::BusConnect(meshBusPair);
            }
            m_meshBusPair = meshBusPair;
        }

        // Everything below the entity picker only makes sense with a component.
        if (QWidget* container = widget())
        {
            const QList<QGroupBox*> groups = container->findChildren<QGroupBox*>();
            for (QGroupBox* group : groups)
            {
                if (group->title() != tr("Entity"))
                {
                    group->setEnabled(hasComponent);
                }
            }
        }
        m_newChildButton->setEnabled(hasComponent);
        m_editButton->setEnabled(hasComponent);

        if (hasComponent)
        {
            // Default shape.
            m_defaultShapeCombo->setCurrentIndex(
                m_defaultShapeCombo->findData(static_cast<int>(component->GetDefaultShape())));

            RefreshLayerControls(component);

            // Draw Shape.
            const DrawShapeType drawShape = component->GetDrawShape();
            m_drawShapeCombo->setCurrentIndex(m_drawShapeCombo->findData(static_cast<int>(drawShape)));
            m_drawSides->setValue(component->GetDrawSides());
            const bool isStair = drawShape == DrawShapeType::Staircase;
            const bool flat = drawShape == DrawShapeType::Plane || drawShape == DrawShapeType::Polygon;
            const bool ringShape = drawShape == DrawShapeType::Torus || drawShape == DrawShapeType::Pipe;
            m_drawHoleRatio->setValue(component->GetDrawHoleRatio());
            m_drawHoleRatio->setVisible(ringShape);
            m_drawHoleRatioLabel->setVisible(ringShape);
            const bool torus = drawShape == DrawShapeType::Torus;
            m_drawTubeSides->setValue(component->GetDrawTubeSides());
            m_drawTubeSides->setVisible(torus);
            m_drawTubeSidesLabel->setVisible(torus);
            m_stairGroup->setVisible(isStair);
            m_drawSides->setVisible(!isStair && !flat);
            m_drawSidesLabel->setVisible(!isStair && !flat);
            const DrawStairInfo stair = component->GetDrawStairInfo();
            m_stairByHeight->setChecked(stair.m_byHeight);
            m_stairSteps->setValue(stair.m_steps);
            m_stairStepHeight->setValue(stair.m_stepHeight);
            m_stairRotation->setValue(stair.m_rotation);
            m_stairSteps->setEnabled(!stair.m_byHeight);
            m_stairStepHeight->setEnabled(stair.m_byHeight);
            m_drawCarve->setChecked(component->GetDrawCarve());
            m_drawMergeUnion->setChecked(component->GetDrawMergeUnion());
            m_drawCarve->setEnabled(!flat);
            m_drawMergeUnion->setEnabled(!flat);

            // Unit Cube Stamp.
            const bool unitCube = component->GetDrawUnitCube();
            m_unitCube->setChecked(unitCube);
            m_unitCubeSize->setValue(component->GetDrawUnitCubeSize());
            m_unitCubeShowGrid->setChecked(component->GetDrawUnitCubeShowGrid());
            m_unitCubeSize->setEnabled(unitCube);
            m_unitCubeShowGrid->setEnabled(unitCube);
            // Clear works whenever stamped cells are recorded, independent of the stamp toggle.
            m_clearCubesButton->setEnabled(true);

            // Boolean.
            m_csgSolverCombo->setCurrentIndex(
                m_csgSolverCombo->findData(static_cast<int>(component->GetCsgSolver())));
            const AZ::EntityId sourceId = component->GetBooleanSourceEntity();
            const int sourceIndex =
                m_booleanSourceCombo->findData(QVariant(static_cast<qulonglong>(static_cast<AZ::u64>(sourceId))));
            m_booleanSourceCombo->setCurrentIndex(sourceIndex >= 0 ? sourceIndex : 0);
            m_booleanOpCombo->setCurrentIndex(
                m_booleanOpCombo->findData(static_cast<int>(component->GetBooleanOperation())));
            m_booleanLive->setChecked(component->GetLiveBoolean());
            m_booleanActiveOnly->setChecked(component->GetBooleanAffectActiveOnly());
            m_booleanSourceAfterCombo->setCurrentIndex(
                m_booleanSourceAfterCombo->findData(static_cast<int>(component->GetSourceAfterApply())));
            const bool hasSource = sourceId.IsValid();
            m_booleanOpCombo->setEnabled(hasSource);
            m_booleanLive->setEnabled(hasSource);
            m_booleanActiveOnly->setEnabled(hasSource);
            m_booleanSourceAfterCombo->setEnabled(hasSource);
            m_applyBooleanButton->setEnabled(hasSource);

            // Global (scene-wide) boolean.
            const bool booleanOthers = component->GetBooleanOthers();
            m_booleanOthers->setChecked(booleanOthers);
            m_cutterOpCombo->setCurrentIndex(
                m_cutterOpCombo->findData(static_cast<int>(component->GetCutterOperation())));
            m_cutterOpCombo->setEnabled(booleanOthers);
            m_excludeFromBoolean->setChecked(component->GetExcludeFromBoolean());

            // Material / display.
            m_defaultMaterial->SetSelectedAssetID(component->GetMaterialOverride());
            const auto paint = component->GetFacePaintSettings();
            m_paintOperation->setCurrentIndex(m_paintOperation->findData(static_cast<int>(paint.m_operation)));
            m_paintMaterial->SetSelectedAssetID(paint.m_material);
            m_paintMaterial->setEnabled(paint.m_operation == FacePaintOperation::Material);
            m_paintColor->setEnabled(paint.m_operation == FacePaintOperation::Color);
            SetColorButtonSwatch(m_paintColor, QColor(
                paint.m_color & 255, (paint.m_color >> 8) & 255, (paint.m_color >> 16) & 255));
            const bool useGlobalTint = component->GetUseGlobalTint();
            m_useGlobalTint->setChecked(useGlobalTint);
            const AZ::Color tint = component->GetMaterialTint();
            SetColorButtonSwatch(m_globalTintButton, QColor::fromRgbF(tint.GetR(), tint.GetG(), tint.GetB()));
            m_globalTintButton->setEnabled(useGlobalTint);
            m_useTexture->setChecked(component->GetMaterialUseTexture());
            m_edgesOnly->setChecked(component->GetEdgesOnly());

            // Mesh ops.
            m_flipYZ->setChecked(component->GetFlipYZForExport());

            // Tool sub-mode (only meaningful while in component mode).
            SubMode subMode = SubMode::Default;
            EditorWhiteBoxComponentModeRequestBus::EventResult(
                subMode, AZ::EntityComponentIdPair(m_currentEntityId, component->GetId()),
                &EditorWhiteBoxComponentModeRequests::GetCurrentSubMode);
            m_modeSketch->setChecked(subMode == SubMode::Default);
            m_modeEdgeRestore->setChecked(subMode == SubMode::EdgeRestore);
            m_modeTransform->setChecked(subMode == SubMode::Transform);
            m_modeDrawShape->setChecked(subMode == SubMode::DrawShape);
            m_modePaint->setChecked(subMode == SubMode::VertexPaint);

            // Layer gizmo: keep it targeting the selected layer.
            m_layerGizmo->SetTarget(m_currentEntityId, m_layerList->currentRow());
            m_layerGizmo->Refresh();

            // Entity gizmo: keep it targeting the current entity.
            m_entityGizmo->SetTarget(m_currentEntityId);
            m_entityGizmo->Refresh();

            // Keep the pane's gizmo buttons (and the viewport cluster highlight) in sync with
            // the actual gizmo modes, wherever they were changed from.
            const auto entityMode = m_entityGizmo->GetMode();
            m_entityGizmoOff->setChecked(entityMode == WhiteBoxEntityGizmo::Mode::None);
            m_entityGizmoMove->setChecked(entityMode == WhiteBoxEntityGizmo::Mode::Translate);
            m_entityGizmoRotate->setChecked(entityMode == WhiteBoxEntityGizmo::Mode::Rotate);
            m_entityGizmoScale->setChecked(entityMode == WhiteBoxEntityGizmo::Mode::Scale);
            UpdateEntityGizmoClusterHighlight();

            const auto layerMode = m_layerGizmo->GetMode();
            m_gizmoOff->setChecked(layerMode == WhiteBoxLayerGizmo::Mode::None);
            m_gizmoMove->setChecked(layerMode == WhiteBoxLayerGizmo::Mode::Translate);
            m_gizmoRotate->setChecked(layerMode == WhiteBoxLayerGizmo::Mode::Rotate);
            m_gizmoScale->setChecked(layerMode == WhiteBoxLayerGizmo::Mode::Scale);
        }
        else
        {
            m_layerGizmo->SetMode(WhiteBoxLayerGizmo::Mode::None);
            m_gizmoOff->setChecked(true);
            m_entityGizmo->SetMode(WhiteBoxEntityGizmo::Mode::None);
            m_entityGizmoOff->setChecked(true);
        }

        m_updating = false;
    }

    void WhiteBoxPaneWidget::SetCurrentEntity(const AZ::EntityId entityId)
    {
        if (entityId == m_currentEntityId)
        {
            return;
        }
        m_currentEntityId = entityId;
        RefreshEntityList(); // re-sync the dropdown (and the boolean source list) to the new entity
        RefreshFromComponent();
    }

    void WhiteBoxPaneWidget::OnEntityComboChanged(const int index)
    {
        if (m_updating || index < 0)
        {
            return;
        }
        const AZ::EntityId entityId(m_entityCombo->itemData(index).toULongLong());
        if (entityId == m_currentEntityId)
        {
            return;
        }
        m_currentEntityId = entityId;

        // Component mode stays latched to the old entity, so end it and re-enter it on the new one.
        namespace Cmf = AzToolsFramework::ComponentModeFramework;
        bool wasInComponentMode = false;
        Cmf::ComponentModeSystemRequestBus::BroadcastResult(
            wasInComponentMode, &Cmf::ComponentModeSystemRequests::InComponentMode);
        if (wasInComponentMode)
        {
            Cmf::ComponentModeSystemRequestBus::Broadcast(&Cmf::ComponentModeSystemRequests::EndComponentMode);
        }

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
            AzToolsFramework::EntityIdList{ entityId });

        if (wasInComponentMode)
        {
            // Deferred a tick so the selection has fully applied before the mode begins.
            AZ::TickBus::QueueFunction(
                [entityId]()
                {
                    if (AZ::Entity* entity = FindEntity(entityId))
                    {
                        if (auto* whiteBox = entity->FindComponent<EditorWhiteBoxComponent>())
                        {
                            whiteBox->EnterComponentMode();
                        }
                    }
                });
        }

        RefreshEntityList();
        RefreshFromComponent();
    }

    void WhiteBoxPaneWidget::CreateWhiteBoxEntity(const bool asChildLayer)
    {
        const AZ::EntityId parentId = asChildLayer ? m_currentEntityId : AZ::EntityId();
        const AZ::EntityId newEntityId = CreateChildWhiteBoxLayer(parentId);
        if (!newEntityId.IsValid())
        {
            return;
        }
        if (!asChildLayer)
        {
            // The layer utility names children "White Box Layer"; a standalone entity is just "White Box".
            if (AZ::Entity* entity = FindEntity(newEntityId))
            {
                entity->SetName("White Box");
            }
        }
        SetCurrentEntity(newEntityId);
    }

    // ---- notifications ----------------------------------------------------------------------

    void WhiteBoxPaneWidget::OnEditorEntityCreated([[maybe_unused]] const AZ::EntityId& entityId)
    {
        RefreshEntityList();
        RefreshFromComponent();
    }

    void WhiteBoxPaneWidget::OnEditorEntityDeleted(const AZ::EntityId& entityId)
    {
        if (entityId == m_currentEntityId)
        {
            m_currentEntityId = AZ::EntityId();
        }
        RefreshEntityList();
        RefreshFromComponent();
    }

    void WhiteBoxPaneWidget::AfterEntitySelectionChanged(
        const AzToolsFramework::EntityIdList& newlySelectedEntities,
        [[maybe_unused]] const AzToolsFramework::EntityIdList& newlyDeselectedEntities)
    {
        // Follow the selection: if a White Box entity is selected, make it the pane's edit target.
        for (const AZ::EntityId& entityId : newlySelectedEntities)
        {
            if (AZ::Entity* entity = FindEntity(entityId))
            {
                if (entity->FindComponent<EditorWhiteBoxComponent>() != nullptr)
                {
                    SetCurrentEntity(entityId);
                    return;
                }
            }
        }
    }

    void WhiteBoxPaneWidget::AfterUndoRedo()
    {
        // Component state may have changed under the pane (layers added/removed, settings undone).
        RefreshEntityList();
        RefreshFromComponent();
    }

    void WhiteBoxPaneWidget::OnEntityComponentAdded(
        [[maybe_unused]] const AZ::EntityId& entityId, [[maybe_unused]] const AZ::ComponentId& componentId)
    {
        RefreshEntityList();
        RefreshFromComponent();
    }

    void WhiteBoxPaneWidget::OnEntityComponentRemoved(
        [[maybe_unused]] const AZ::EntityId& entityId, [[maybe_unused]] const AZ::ComponentId& componentId)
    {
        RefreshEntityList();
        RefreshFromComponent();
    }

    void WhiteBoxPaneWidget::EnsureEnabledInComponentMode()
    {
        // Deferred so it runs AFTER whoever is disabling the dock panes has finished.
        QTimer::singleShot(
            0, this,
            [this]()
            {
                namespace Cmf = AzToolsFramework::ComponentModeFramework;
                bool inComponentMode = false;
                Cmf::ComponentModeSystemRequestBus::BroadcastResult(
                    inComponentMode, &Cmf::ComponentModeSystemRequests::InComponentMode);
                if (!inComponentMode)
                {
                    return; // respect the disable outside component mode (e.g. game/sim mode)
                }
                for (QWidget* w = this; w != nullptr; w = w->parentWidget())
                {
                    if (!w->isEnabled())
                    {
                        w->setEnabled(true);
                    }
                    // The editor also dims disabled panes with a translucent graphics effect
                    // (it doesn't block input, it just LOOKS gray) - remove it from our chain.
                    if (w->graphicsEffect() != nullptr)
                    {
                        w->setGraphicsEffect(nullptr);
                    }
                }
                // The dimming effect may sit on our child widgets as well - clear those too.
                if (QWidget* container = widget())
                {
                    const QList<QWidget*> children = container->findChildren<QWidget*>();
                    for (QWidget* child : children)
                    {
                        if (child->graphicsEffect() != nullptr)
                        {
                            child->setGraphicsEffect(nullptr);
                        }
                    }
                }
                RefreshFromComponent(); // pick up the active sub-mode for the Tool Mode buttons
            });
    }

    void WhiteBoxPaneWidget::changeEvent(QEvent* event)
    {
        QScrollArea::changeEvent(event);
        // The editor disables dock panes in component mode; this pane is the editing UI, so undo that.
        if (event->type() == QEvent::EnabledChange && !isEnabled())
        {
            EnsureEnabledInComponentMode();
        }
    }

    void WhiteBoxPaneWidget::OnWhiteBoxMeshModified()
    {
        // Defer a tick (the component may auto-create a layer on the same bus) and rebuild only if the
        // layer structure changed.
        QTimer::singleShot(
            0, this,
            [this]()
            {
                if (EditorWhiteBoxComponent* component = CurrentComponent())
                {
                    if (component->GetLayerCount() != m_layerList->count() ||
                        component->GetLayerCount() != m_activeLayerCombo->count())
                    {
                        RefreshFromComponent();
                    }
                }
            });
    }

    void WhiteBoxPaneWidget::OnDefaultShapeTypeChanged([[maybe_unused]] DefaultShapeType defaultShape)
    {
        RefreshFromComponent();
    }

    void WhiteBoxPaneWidget::OnLayerStructureChanged()
    {
        // Deferred one event-loop tick: this arrives mid SyncLayerStructure, before the
        // component has finished settling its working state.
        QTimer::singleShot(0, this, [this]() { RefreshFromComponent(); });
    }

    void WhiteBoxPaneWidget::CreateEntityGizmoCluster()
    {
        namespace Vui = AzToolsFramework::ViewportUi;
        if (m_entityGizmoClusterId != Vui::ClusterId{})
        {
            return; // already shown
        }

        Vui::ViewportUiRequestBus::Event(
            Vui::DefaultViewportId,
            [this](Vui::ViewportUiRequests* requests)
            {
                const auto fetchIcon = [](const char* iconName)
                {
                    return AZStd::string::format(":/stylesheet/img/UI20/toolbar/%s.svg", iconName);
                };
                m_entityGizmoClusterId = requests->CreateCluster(Vui::Alignment::TopLeft);
                m_entityGizmoMoveButtonId = requests->CreateClusterButton(m_entityGizmoClusterId, fetchIcon("Move"));
                m_entityGizmoRotateButtonId = requests->CreateClusterButton(m_entityGizmoClusterId, fetchIcon("Rotate"));
                m_entityGizmoScaleButtonId = requests->CreateClusterButton(m_entityGizmoClusterId, fetchIcon("Scale"));
                requests->SetClusterButtonTooltip(
                    m_entityGizmoClusterId, m_entityGizmoMoveButtonId, "Move the ENTITY (White Box pane gizmo)");
                requests->SetClusterButtonTooltip(
                    m_entityGizmoClusterId, m_entityGizmoRotateButtonId, "Rotate the ENTITY (White Box pane gizmo)");
                requests->SetClusterButtonTooltip(
                    m_entityGizmoClusterId, m_entityGizmoScaleButtonId, "Scale the ENTITY (White Box pane gizmo)");
            });

        m_entityGizmoClusterHandler = AZ::Event<Vui::ButtonId>::Handler(
            [this](Vui::ButtonId buttonId)
            {
                WhiteBoxEntityGizmo::Mode mode = WhiteBoxEntityGizmo::Mode::None;
                if (buttonId == m_entityGizmoMoveButtonId)
                {
                    mode = WhiteBoxEntityGizmo::Mode::Translate;
                }
                else if (buttonId == m_entityGizmoRotateButtonId)
                {
                    mode = WhiteBoxEntityGizmo::Mode::Rotate;
                }
                else if (buttonId == m_entityGizmoScaleButtonId)
                {
                    mode = WhiteBoxEntityGizmo::Mode::Scale;
                }
                // Clicking the active mode's button again toggles the gizmo off.
                if (m_entityGizmo->GetMode() == mode)
                {
                    mode = WhiteBoxEntityGizmo::Mode::None;
                }
                m_entityGizmo->SetTarget(m_currentEntityId);
                m_entityGizmo->SetMode(mode);
                UpdateEntityGizmoClusterHighlight();
                RefreshFromComponent(); // sync the pane's gizmo buttons
            });
        Vui::ViewportUiRequestBus::Event(
            Vui::DefaultViewportId, &Vui::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_entityGizmoClusterId, m_entityGizmoClusterHandler);
    }

    void WhiteBoxPaneWidget::RemoveEntityGizmoCluster()
    {
        namespace Vui = AzToolsFramework::ViewportUi;
        if (m_entityGizmoClusterId != Vui::ClusterId{})
        {
            Vui::ViewportUiRequestBus::Event(
                Vui::DefaultViewportId, &Vui::ViewportUiRequestBus::Events::RemoveCluster, m_entityGizmoClusterId);
            m_entityGizmoClusterId = Vui::ClusterId{};
        }
    }

    void WhiteBoxPaneWidget::UpdateEntityGizmoClusterHighlight()
    {
        namespace Vui = AzToolsFramework::ViewportUi;
        if (m_entityGizmoClusterId == Vui::ClusterId{})
        {
            return;
        }
        Vui::ButtonId activeButton{};
        switch (m_entityGizmo->GetMode())
        {
        case WhiteBoxEntityGizmo::Mode::Translate:
            activeButton = m_entityGizmoMoveButtonId;
            break;
        case WhiteBoxEntityGizmo::Mode::Rotate:
            activeButton = m_entityGizmoRotateButtonId;
            break;
        case WhiteBoxEntityGizmo::Mode::Scale:
            activeButton = m_entityGizmoScaleButtonId;
            break;
        default:
            break;
        }
        if (activeButton != Vui::ButtonId{})
        {
            Vui::ViewportUiRequestBus::Event(
                Vui::DefaultViewportId, &Vui::ViewportUiRequestBus::Events::SetClusterActiveButton,
                m_entityGizmoClusterId, activeButton);
        }
        else
        {
            Vui::ViewportUiRequestBus::Event(
                Vui::DefaultViewportId, &Vui::ViewportUiRequestBus::Events::ClearClusterActiveButton,
                m_entityGizmoClusterId);
        }
    }

    void WhiteBoxPaneWidget::OnEditorModeActivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
        const AzToolsFramework::ViewportEditorMode mode)
    {
        if (mode == AzToolsFramework::ViewportEditorMode::Component)
        {
            EnsureEnabledInComponentMode();
            // The editor's own entity gizmo is suppressed during component mode; offer ours
            // via an in-viewport Move/Rotate/Scale cluster (like the tool-mode clusters).
            CreateEntityGizmoCluster();
        }
    }

    void WhiteBoxPaneWidget::OnEditorModeDeactivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
        const AzToolsFramework::ViewportEditorMode mode)
    {
        if (mode == AzToolsFramework::ViewportEditorMode::Component)
        {
            // Leaving edit mode: drop the cluster and the gizmo (the editor's own entity
            // gizmo takes over again outside component mode).
            RemoveEntityGizmoCluster();
            m_entityGizmo->SetMode(WhiteBoxEntityGizmo::Mode::None);
            RefreshFromComponent();
        }
    }
} // namespace WhiteBox
