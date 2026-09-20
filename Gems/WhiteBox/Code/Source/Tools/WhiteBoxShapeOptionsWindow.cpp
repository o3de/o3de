/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "Tools/WhiteBoxShapeOptionsWindow.h"

#include "EditorWhiteBoxComponent.h"
#include "Util/WhiteBoxEditorUtil.h"
#include "Viewport/WhiteBoxShapeBuilders.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

namespace WhiteBox
{
    namespace
    {

        QString ShapeName(const DrawShapeType shape)
        {
            switch (shape)
            {
            case DrawShapeType::Box: return QObject::tr("Box");
            case DrawShapeType::Cylinder: return QObject::tr("Cylinder");
            case DrawShapeType::Pyramid: return QObject::tr("Pyramid");
            case DrawShapeType::Cone: return QObject::tr("Cone");
            case DrawShapeType::Sphere: return QObject::tr("Sphere");
            case DrawShapeType::Staircase: return QObject::tr("Staircase");
            case DrawShapeType::Room: return QObject::tr("Room");
            case DrawShapeType::Door: return QObject::tr("Door");
            case DrawShapeType::CircularStairs: return QObject::tr("Circular Stairs");
            case DrawShapeType::Plane: return QObject::tr("Plane");
            case DrawShapeType::Torus: return QObject::tr("Torus");
            case DrawShapeType::Pipe: return QObject::tr("Pipe");
            case DrawShapeType::Polygon: return QObject::tr("Freeform Polygon");
            default: return QObject::tr("Shape");
            }
        }
    } // namespace

    void WhiteBoxShapeOptionsWindow::Row::SetVisible(const bool visible) const
    {
        if (m_label)
        {
            m_label->setVisible(visible);
        }
        if (m_field)
        {
            m_field->setVisible(visible);
        }
    }

    WhiteBoxShapeOptionsWindow::WhiteBoxShapeOptionsWindow(const AZ::EntityComponentIdPair& pair, QWidget* parent)
        : WhiteBoxModelingWindow(tr("Shape Options"), parent)
        , m_pair(pair)
    {
        setObjectName("WhiteBoxShapeOptionsWindow");

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(1, 1, 1, 10);
        layout->setSpacing(10);

        auto* header = new QWidget(this);
        header->setObjectName("ShapeOptionsHeader");
        SetDragHandle(header);
        auto* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(12, 9, 12, 9);
        m_title = new QLabel(tr("Shape"), header);
        m_title->setObjectName("ShapeOptionsTitle");
        headerLayout->addWidget(m_title);
        headerLayout->addStretch();
        layout->addWidget(header);

        auto* controls = new QGridLayout();
        controls->setContentsMargins(12, 0, 12, 0);
        controls->setHorizontalSpacing(9);
        controls->setVerticalSpacing(7);

        int row = 0;
        const auto addRow = [&](const QString& label, QWidget* field, const QString& tip) -> Row
        {
            auto* text = new QLabel(label, this);
            field->setToolTip(tip);
            controls->addWidget(text, row, 0);
            controls->addWidget(field, row, 1);
            ++row;
            return Row{ text, field };
        };

        m_sides = new QSpinBox(this);
        m_sides->setRange(3, 128);
        m_sidesRow = addRow(
            tr("Sides"), m_sides,
            tr("Sides for round and N-gon solids (4 gives a box or square), or the subdivision of a Sphere."));

        m_tubeSides = new QSpinBox(this);
        m_tubeSides->setRange(MinTubeSides, MaxTubeSides);
        m_tubeSidesRow = addRow(
            tr("Tube Sides"), m_tubeSides,
            tr("Segments around the torus tube's cross-section. A torus is Sides x Tube Sides quads, so "
               "this is the other half of its triangle count."));

        m_holeRatio = new QDoubleSpinBox(this);
        m_holeRatio->setDecimals(2);
        m_holeRatio->setRange(0.05, 0.95);
        m_holeRatio->setSingleStep(0.05);
        m_holeRatioRow = addRow(
            tr("Hole Ratio"), m_holeRatio,
            tr("Hole diameter as a fraction of the outer diameter. Higher values give a thinner wall or tube."));

        m_steps = new QSpinBox(this);
        m_steps->setRange(1, 128);
        m_stepsRow = addRow(tr("Steps"), m_steps, tr("Number of steps in the staircase."));

        m_stepHeight = new QDoubleSpinBox(this);
        m_stepHeight->setDecimals(3);
        m_stepHeight->setRange(0.01, 1000.0);
        m_stepHeight->setSingleStep(0.05);
        m_stepHeightRow = addRow(
            tr("Step Height"), m_stepHeight,
            tr("Target riser height. The step count is rounded to fit the drawn height."));

        m_cubeSize = new QDoubleSpinBox(this);
        m_cubeSize->setDecimals(3);
        m_cubeSize->setRange(0.05, 100.0);
        m_cubeSize->setSingleStep(0.25);
        m_cubeSizeRow = addRow(
            tr("Cube Size"), m_cubeSize,
            tr("World size of the next stamped cube. The stamp grid snaps to this."));

        layout->addLayout(controls);

        auto* toggles = new QVBoxLayout();
        toggles->setContentsMargins(12, 0, 12, 0);
        toggles->setSpacing(5);
        m_stepsByHeight = new QCheckBox(tr("Divide by step height"), this);
        m_cubeShowGrid = new QCheckBox(tr("Show stamp grid"), this);
        m_cubeShowGrid->setToolTip(tr("Draw the per-cube grid in the stamp preview."));
        m_carve = new QCheckBox(tr("Carve (boolean)"), this);
        m_carve->setToolTip(tr("Drawing performs a CSG boolean, the same as holding Ctrl: pull in to carve, out to add."));
        m_mergeUnion = new QCheckBox(tr("Merge into mesh (union)"), this);
        m_mergeUnion->setToolTip(tr("Committing the drawn shape unions it into the mesh instead of leaving overlapping geometry."));
        m_polygonExtrude = new QCheckBox(tr("Extrude after closing"), this);
        m_polygonExtrude->setToolTip(tr(
            "Close the polygon, then move the mouse or type a depth. Click or press Enter to finish; Esc cancels."));
        toggles->addWidget(m_polygonExtrude);
        toggles->addWidget(m_stepsByHeight);
        toggles->addWidget(m_cubeShowGrid);
        toggles->addWidget(m_carve);
        toggles->addWidget(m_mergeUnion);
        layout->addLayout(toggles);

        const auto apply = [this]()
        {
            ApplyValues();
        };
        connect(m_sides, QOverload<int>::of(&QSpinBox::valueChanged), this, apply);
        connect(m_tubeSides, QOverload<int>::of(&QSpinBox::valueChanged), this, apply);
        connect(m_steps, QOverload<int>::of(&QSpinBox::valueChanged), this, apply);
        connect(m_holeRatio, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, apply);
        connect(m_stepHeight, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, apply);
        connect(m_stepsByHeight, &QCheckBox::toggled, this, apply);
        connect(m_cubeSize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, apply);
        connect(m_cubeShowGrid, &QCheckBox::toggled, this, apply);
        connect(m_carve, &QCheckBox::toggled, this, apply);
        connect(m_mergeUnion, &QCheckBox::toggled, this, apply);
        connect(m_polygonExtrude, &QCheckBox::toggled, this, apply);

        RefreshValues();
    }

    EditorWhiteBoxComponent* WhiteBoxShapeOptionsWindow::CurrentComponent() const
    {
        return FindWhiteBoxComponent(m_pair);
    }

    void WhiteBoxShapeOptionsWindow::ApplyVisibility(const DrawShapeType shape, const bool cubeStamp)
    {
        // The cube stamp replaces the primitive entirely, so none of the shape rows apply to it.
        const bool stair = !cubeStamp && (shape == DrawShapeType::Staircase || shape == DrawShapeType::CircularStairs);
        const bool ring = !cubeStamp && (shape == DrawShapeType::Torus || shape == DrawShapeType::Pipe);
        const bool flat = shape == DrawShapeType::Plane || shape == DrawShapeType::Polygon;
        const bool byHeight = m_stepsByHeight->isChecked();

        m_cubeSizeRow.SetVisible(cubeStamp);
        m_cubeShowGrid->setVisible(cubeStamp);

        m_sidesRow.SetVisible(!cubeStamp && !stair && !flat);
        m_sidesRow.m_label->setText(shape == DrawShapeType::Sphere ? tr("Subdivision") : tr("Sides"));
        m_tubeSidesRow.SetVisible(!cubeStamp && shape == DrawShapeType::Torus);
        m_holeRatioRow.SetVisible(ring);
        m_stepsRow.SetVisible(stair && !byHeight);
        m_stepHeightRow.SetVisible(stair && byHeight);
        m_stepsByHeight->setVisible(stair);
        m_polygonExtrude->setVisible(!cubeStamp && shape == DrawShapeType::Polygon);
        const bool polygon = !cubeStamp && shape == DrawShapeType::Polygon;
        const bool booleanAvailable = !polygon || m_polygonExtrude->isChecked();
        m_carve->setVisible(shape != DrawShapeType::Plane || cubeStamp);
        m_mergeUnion->setVisible(shape != DrawShapeType::Plane || cubeStamp);
        m_carve->setEnabled(booleanAvailable);
        m_mergeUnion->setEnabled(booleanAvailable);
        m_carve->setToolTip(polygon && !booleanAvailable
            ? tr("Enable Extrude after closing to carve with a solid polygon.")
            : tr("Pull inward to subtract, outward to add. Takes precedence over Merge into mesh."));
        m_mergeUnion->setToolTip(polygon && !booleanAvailable
            ? tr("Enable Extrude after closing to merge a solid polygon into the mesh.")
            : tr("Union the drawn solid into the active mesh in either pull direction. Turn Carve off to force a union."));

        // A floating palette that keeps the height of its tallest primitive reads as broken, so it
        // shrinks back to whatever is actually on show.
        adjustSize();
    }

    void WhiteBoxShapeOptionsWindow::RefreshValues()
    {
        auto* component = CurrentComponent();
        if (component == nullptr)
        {
            Dismiss();
            return;
        }

        const DrawShapeType shape = component->GetDrawShape();
        const DrawStairInfo stair = component->GetDrawStairInfo();

        m_updating = true;
        {
            const QSignalBlocker sidesBlock(m_sides), tubeBlock(m_tubeSides), stepsBlock(m_steps);
            const QSignalBlocker holeBlock(m_holeRatio), stepHeightBlock(m_stepHeight);
            const QSignalBlocker byHeightBlock(m_stepsByHeight), carveBlock(m_carve), mergeBlock(m_mergeUnion);
            const QSignalBlocker polygonExtrudeBlock(m_polygonExtrude);
            m_sides->setValue(component->GetDrawSides());
            m_tubeSides->setValue(component->GetDrawTubeSides());
            m_holeRatio->setValue(component->GetDrawHoleRatio());
            m_steps->setValue(stair.m_steps);
            m_stepHeight->setValue(stair.m_stepHeight);
            m_stepsByHeight->setChecked(stair.m_byHeight);
            m_carve->setChecked(component->GetDrawCarve());
            m_mergeUnion->setChecked(component->GetDrawMergeUnion());
            m_polygonExtrude->setChecked(component->GetDrawPolygonExtrude());
            m_cubeSize->setValue(component->GetDrawUnitCubeSize());
            m_cubeShowGrid->setChecked(component->GetDrawUnitCubeShowGrid());
        }
        m_updating = false;

        const bool cubeStamp = component->GetDrawUnitCube();
        m_title->setText(cubeStamp ? tr("Cube Stamp") : ShapeName(shape));
        ApplyVisibility(shape, cubeStamp);
    }

    void WhiteBoxShapeOptionsWindow::ApplyValues()
    {
        if (m_updating)
        {
            return;
        }
        auto* component = CurrentComponent();
        if (component == nullptr)
        {
            Dismiss();
            return;
        }

        // These are serialized tool settings, so each change is its own undo step - the same as
        // editing them in the pane.
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Shape Options");
            component->SetDrawSides(m_sides->value());
            component->SetDrawTubeSides(m_tubeSides->value());
            component->SetDrawHoleRatio(static_cast<float>(m_holeRatio->value()));
            component->SetDrawCarve(m_carve->isChecked());
            component->SetDrawMergeUnion(m_mergeUnion->isChecked());
            component->SetDrawPolygonExtrude(m_polygonExtrude->isChecked());

            DrawStairInfo stair = component->GetDrawStairInfo();
            stair.m_steps = m_steps->value();
            stair.m_stepHeight = static_cast<float>(m_stepHeight->value());
            stair.m_byHeight = m_stepsByHeight->isChecked();
            component->SetDrawStairInfo(stair);

            component->SetDrawUnitCubeSize(static_cast<float>(m_cubeSize->value()));
            component->SetDrawUnitCubeShowGrid(m_cubeShowGrid->isChecked());

            undoBatch.MarkEntityDirty(m_pair.GetEntityId());
        }

        // Toggling "divide by step height" swaps which of the two step rows is relevant.
        ApplyVisibility(component->GetDrawShape(), component->GetDrawUnitCube());
    }
} // namespace WhiteBox
