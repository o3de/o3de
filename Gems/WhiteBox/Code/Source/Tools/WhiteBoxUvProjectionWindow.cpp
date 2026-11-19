/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Tools/WhiteBoxUvProjectionWindow.h"
#include "Tools/WhiteBoxUvEditorPane.h"
#include "EditorWhiteBoxComponent.h"
#include "SubComponentModes/EditorWhiteBoxTransformModeBus.h"
#include "Util/WhiteBoxEditorUtil.h"
#include "Util/WhiteBoxModelingOps.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace WhiteBox
{
    WhiteBoxUvProjectionWindow::WhiteBoxUvProjectionWindow(const AZ::EntityComponentIdPair& pair, QWidget* parent)
        : WhiteBoxModelingWindow(tr("UV Projection"), parent)
        , m_pair(pair)
    {
        setObjectName("WhiteBoxUvProjectionWindow");
        if (auto* component = FindWhiteBoxComponent(m_pair))
        {
            m_layerId = component->GetActiveLayerId();
        }

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(12, 9, 12, 10);
        layout->setSpacing(10);
        auto* title = new QLabel(tr("UV Projection"), this);
        SetDragHandle(title);
        layout->addWidget(title);

        auto* controls = new QGridLayout();
        controls->setHorizontalSpacing(9);
        controls->setVerticalSpacing(7);
        m_mode = new QComboBox(this);
        m_mode->addItem(tr("World (box)"), static_cast<int>(Api::UvProjectionMode::World));
        m_mode->addItem(tr("Planar (face)"), static_cast<int>(Api::UvProjectionMode::Planar));
        m_mode->addItem(tr("Manual (keep UVs)"), static_cast<int>(Api::UvProjectionMode::Manual));
        m_mode->setToolTip(
            tr("World projects along each face's main axis, as White Box always has. Planar follows the face itself, so "
               "slopes do not stretch. Manual freezes the current UVs: edits keep them, and faces an edit creates continue "
               "them. Pasting a Manual projection carries its UVs across."));
        const auto makeSpin = [this](const double minimum, const double maximum, const double step, const int decimals)
        {
            auto* spin = new QDoubleSpinBox(this);
            spin->setRange(minimum, maximum);
            spin->setSingleStep(step);
            spin->setDecimals(decimals);
            // Typed values apply when finished; arrow steps apply as they happen.
            spin->setKeyboardTracking(false);
            return spin;
        };
        // Fit to Face can produce small tilings and large offsets on big or distant faces, so the ranges are wide.
        m_tilingU = makeSpin(0.0001, 1000.0, 0.1, 4);
        m_tilingV = makeSpin(0.0001, 1000.0, 0.1, 4);
        m_offsetU = makeSpin(-100000.0, 100000.0, 0.05, 4);
        m_offsetV = makeSpin(-100000.0, 100000.0, 0.05, 4);
        m_rotation = makeSpin(-360.0, 360.0, 15.0, 1);
        m_rotation->setSuffix(tr(" deg"));
        m_tilingU->setToolTip(tr("Texture repeats per metre along U."));
        m_tilingV->setToolTip(tr("Texture repeats per metre along V."));
        controls->addWidget(new QLabel(tr("Mode"), this), 0, 0);
        controls->addWidget(m_mode, 0, 1, 1, 2);
        controls->addWidget(new QLabel(tr("Tiling"), this), 1, 0);
        controls->addWidget(m_tilingU, 1, 1);
        controls->addWidget(m_tilingV, 1, 2);
        controls->addWidget(new QLabel(tr("Offset"), this), 2, 0);
        controls->addWidget(m_offsetU, 2, 1);
        controls->addWidget(m_offsetV, 2, 2);
        controls->addWidget(new QLabel(tr("Rotation"), this), 3, 0);
        controls->addWidget(m_rotation, 3, 1, 1, 2);
        // Density is not a live control: it only applies when Normalize is pressed.
        m_density = makeSpin(0.0001, 1000.0, 0.1, 4);
        m_density->setToolTip(tr("Repeats per metre that Normalize gives every selected polygon. Starts at their average."));
        m_normalize = new QPushButton(tr("Normalize"), this);
        m_normalize->setToolTip(tr("Give every selected polygon this tiling on both axes, so the texture reads at one scale across them."));
        controls->addWidget(new QLabel(tr("Density"), this), 4, 0);
        controls->addWidget(m_density, 4, 1);
        controls->addWidget(m_normalize, 4, 2);
        layout->addLayout(controls);

        m_selection = new QLabel(this);
        layout->addWidget(m_selection);
        m_status = new QLabel(this);
        m_status->setWordWrap(true);
        m_status->hide();
        layout->addWidget(m_status);

        auto* actions = new QHBoxLayout();
        m_fit = new QPushButton(tr("Fit to Face"), this);
        m_fit->setToolTip(tr("Scale and offset each selected polygon's texture so it covers the polygon exactly once."));
        m_reset = new QPushButton(tr("Reset"), this);
        m_reset->setToolTip(tr("Back to the default World mapping."));
        auto* close = new QPushButton(tr("Close  [Esc]"), this);
        m_copy = new QPushButton(tr("Copy"), this);
        m_copy->setToolTip(tr("Remember the first selected polygon's projection."));
        m_paste = new QPushButton(tr("Paste"), this);
        m_paste->setToolTip(tr("Apply the copied projection to the selected polygons, on this or any other White Box."));
        // Enter commits a typed value; with no default button it never closes the window as well.
        for (auto* button : { m_fit, m_reset, close, m_normalize, m_copy, m_paste })
        {
            button->setAutoDefault(false);
            button->setDefault(false);
        }
        auto* editor = new QPushButton(tr("Open UV Editor"), this);
        editor->setToolTip(tr("Edit the selected polygons' UVs point by point. Edited faces switch to Manual."));
        editor->setAutoDefault(false);
        editor->setDefault(false);
        auto* clipboard = new QHBoxLayout();
        clipboard->addWidget(m_copy);
        clipboard->addWidget(m_paste);
        clipboard->addWidget(editor);
        layout->addLayout(clipboard);
        connect(editor, &QPushButton::clicked, this, []() { AzToolsFramework::OpenViewPane(WhiteBoxUvEditorPane::PaneName); });
        actions->addWidget(m_fit);
        actions->addWidget(m_reset);
        actions->addWidget(close);
        layout->addLayout(actions);

        connect(m_mode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { Apply(); });
        for (auto* spin : { m_tilingU, m_tilingV, m_offsetU, m_offsetV, m_rotation })
        {
            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) { Apply(); });
        }
        connect(m_fit, &QPushButton::clicked, this, [this]()
        {
            const auto result = ModelingOps::FitUvProjection(m_pair);
            ShowResult(result.m_success, result.m_message);
            RefreshSelection(true);
        });
        connect(m_reset, &QPushButton::clicked, this, [this]()
        {
            const auto result = ModelingOps::ApplyUvProjection(m_pair, Api::UvProjection{});
            ShowResult(result.m_success, result.m_message);
            RefreshSelection(true);
        });
        connect(m_normalize, &QPushButton::clicked, this, [this]()
        {
            const auto result = ModelingOps::NormalizeTexelDensity(m_pair, static_cast<float>(m_density->value()));
            ShowResult(result.m_success, result.m_message);
            RefreshSelection(true);
        });
        connect(m_copy, &QPushButton::clicked, this, [this]()
        {
            const auto result = ModelingOps::CopyUvProjection(m_pair);
            ShowResult(result.m_success, result.m_message);
        });
        connect(m_paste, &QPushButton::clicked, this, [this]()
        {
            const auto result = ModelingOps::PasteUvProjection(m_pair);
            ShowResult(result.m_success, result.m_message);
            RefreshSelection(true);
        });
        connect(close, &QPushButton::clicked, this, &WhiteBoxUvProjectionWindow::reject);

        auto* refresh = new QTimer(this);
        connect(refresh, &QTimer::timeout, this, [this]() { RefreshSelection(); });
        refresh->start(150);
        RefreshSelection(true);
    }

    bool WhiteBoxUvProjectionWindow::HasCurrentLayer() const
    {
        auto* component = FindWhiteBoxComponent(m_pair);
        return component && component->GetWhiteBoxMesh() && component->GetActiveLayerId() == m_layerId;
    }

    void WhiteBoxUvProjectionWindow::RefreshSelection(const bool force)
    {
        if (!HasCurrentLayer())
        {
            Dismiss();
            return;
        }
        Api::PolygonHandles polygons;
        EditorWhiteBoxTransformModeRequestBus::EventResult(
            polygons, m_pair, &EditorWhiteBoxTransformModeRequests::GetSelectedPolygons);
        m_selection->setText(polygons.empty() ? tr("Select polygons in Transform mode.")
                                              : tr("%n polygon(s) selected", nullptr, static_cast<int>(polygons.size())));
        const bool usable = !polygons.empty();
        // Tiling, offset, rotation, Fit and density shape a projection; Manual UVs have none to shape.
        const auto projection = ModelingOps::SelectedUvProjection(m_pair);
        const bool manual = projection && projection->m_mode == Api::UvProjectionMode::Manual;
        for (QWidget* control : std::initializer_list<QWidget*>{ m_mode, m_reset, m_copy })
        {
            control->setEnabled(usable);
        }
        for (QWidget* control : std::initializer_list<QWidget*>{
                 m_tilingU, m_tilingV, m_offsetU, m_offsetV, m_rotation, m_fit, m_density, m_normalize })
        {
            control->setEnabled(usable && !manual);
        }
        m_paste->setEnabled(usable && ModelingOps::HasCopiedUvProjection());
        // Follows selection changes and undo, but never overwrites a value while it is being typed.
        const bool editing = m_tilingU->hasFocus() || m_tilingV->hasFocus() || m_offsetU->hasFocus() || m_offsetV->hasFocus() ||
            m_rotation->hasFocus();
        if ((force || !editing) && projection)
        {
            LoadValues(*projection);
        }
        if ((force || !m_density->hasFocus()) && usable)
        {
            if (const auto density = ModelingOps::SelectedTexelDensity(m_pair))
            {
                const QSignalBlocker densityBlock(m_density);
                m_density->setValue(*density);
            }
        }
    }

    void WhiteBoxUvProjectionWindow::LoadValues(const Api::UvProjection& projection)
    {
        const QSignalBlocker modeBlock(m_mode), tilingUBlock(m_tilingU), tilingVBlock(m_tilingV), offsetUBlock(m_offsetU),
            offsetVBlock(m_offsetV), rotationBlock(m_rotation);
        m_mode->setCurrentIndex(m_mode->findData(static_cast<int>(projection.m_mode)));
        m_tilingU->setValue(projection.m_scale.GetX());
        m_tilingV->setValue(projection.m_scale.GetY());
        m_offsetU->setValue(projection.m_offset.GetX());
        m_offsetV->setValue(projection.m_offset.GetY());
        m_rotation->setValue(projection.m_rotationDegrees);
    }

    Api::UvProjection WhiteBoxUvProjectionWindow::ControlValues() const
    {
        Api::UvProjection projection;
        projection.m_mode = static_cast<Api::UvProjectionMode>(m_mode->currentData().toInt());
        projection.m_scale = AZ::Vector2(static_cast<float>(m_tilingU->value()), static_cast<float>(m_tilingV->value()));
        projection.m_offset = AZ::Vector2(static_cast<float>(m_offsetU->value()), static_cast<float>(m_offsetV->value()));
        projection.m_rotationDegrees = static_cast<float>(m_rotation->value());
        return projection;
    }

    void WhiteBoxUvProjectionWindow::Apply()
    {
        // Revalidate at the edit, not just on the timer, so switching layers is safe.
        if (!HasCurrentLayer())
        {
            Dismiss();
            return;
        }
        const auto result = ModelingOps::ApplyUvProjection(m_pair, ControlValues());
        ShowResult(result.m_success, result.m_message);
    }

    void WhiteBoxUvProjectionWindow::ShowResult(const bool success, const AZStd::string& message)
    {
        // Success is visible in the viewport; only a refusal needs words.
        m_status->setText(QString::fromUtf8(message.c_str()));
        m_status->setVisible(!success && !message.empty());
        adjustSize();
    }

    void WhiteBoxUvProjectionWindow::reject()
    {
        Dismiss();
    }
} // namespace WhiteBox
