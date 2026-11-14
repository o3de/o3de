/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "Tools/WhiteBoxExtrudeInsetWindow.h"
#include "EditorWhiteBoxComponent.h"
#include "SubComponentModes/EditorWhiteBoxTransformModeBus.h"
#include "Util/WhiteBoxEditorUtil.h"
#include "Util/WhiteBoxModelingOps.h"

#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QSignalBlocker>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace WhiteBox
{
    WhiteBoxExtrudeInsetWindow::WhiteBoxExtrudeInsetWindow(
        const AZ::EntityComponentIdPair& pair, const bool inset, QWidget* parent)
        : WhiteBoxModelingWindow(inset ? tr("Inset") : tr("Extrude"), parent)
        , m_pair(pair)
        , m_inset(inset)
    {
        if (auto* component = FindWhiteBoxComponent(pair)) { m_layerId = component->GetActiveLayerId(); }
        auto* layout = new QVBoxLayout(this);
        auto* title = new QLabel(inset ? tr("Inset Region") : tr("Extrude Region"), this);
        SetDragHandle(title);
        layout->addWidget(title);
        m_latch = new QCheckBox(inset ? tr("Latch: drag polygons to inset") : tr("Latch: drag polygons or edges to extrude"), this);
        m_latch->setToolTip(tr("Stays enabled for repeated drags. Uncheck to return to normal Transform manipulation."));
        layout->addWidget(m_latch);
        connect(m_latch, &QCheckBox::toggled, this, [this](bool enabled)
        {
            EditorWhiteBoxTransformModeRequestBus::Event(
                m_pair, &EditorWhiteBoxTransformModeRequests::SetModelingLatch,
                enabled ? (m_inset ? TransformModelingLatch::Inset : TransformModelingLatch::Extrude)
                        : TransformModelingLatch::None);
        });
        // Armed on open: the button that opened this window is the Extrude/Inset verb, so the drag it
        // enables should already work. The amount below is the other way in, not a prerequisite.
        m_latch->setChecked(true);
        auto* row = new QHBoxLayout();
        row->addWidget(new QLabel(inset ? tr("Inset") : tr("Distance"), this));
        m_amount = new QDoubleSpinBox(this);
        m_amount->setDecimals(inset ? 1 : 3);
        m_amount->setRange(inset ? 0.1 : -10000.0, inset ? 99.0 : 10000.0);
        m_amount->setSingleStep(inset ? 1.0 : 0.1);
        m_amount->setValue(inset ? 10.0 : 0.1);
        if (inset) { m_amount->setSuffix(tr(" %")); }
        m_amount->setToolTip(inset
            ? tr("Inset each connected region, keeping its internal polygon divisions. Flat convex regions shrink toward "
                 "their centre; concave, holed or bent ones get an even border, where 100% is as deep as it can go.")
            : tr("Move each connected region along its average normal. Negative values extrude inward."));
        row->addWidget(m_amount);
        layout->addLayout(row);
        m_selection = new QLabel(this);
        layout->addWidget(m_selection);
        m_status = new QLabel(this);
        m_status->setWordWrap(true);
        m_status->hide();
        layout->addWidget(m_status);
        auto* actions = new QHBoxLayout();
        m_apply = new QPushButton(inset ? tr("Inset  [Enter]") : tr("Extrude  [Enter]"), this);
        m_apply->setDefault(true);
        auto* cancel = new QPushButton(tr("Cancel  [Esc]"), this);
        cancel->setAutoDefault(false);
        actions->addWidget(m_apply);
        actions->addWidget(cancel);
        layout->addLayout(actions);
        connect(m_apply, &QPushButton::clicked, this, &WhiteBoxExtrudeInsetWindow::accept);
        connect(cancel, &QPushButton::clicked, this, &WhiteBoxExtrudeInsetWindow::reject);
        auto* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [this]() { RefreshSelection(); });
        timer->start(150);
        RefreshSelection();
    }

    bool WhiteBoxExtrudeInsetWindow::HasCurrentLayer() const
    {
        auto* component = FindWhiteBoxComponent(m_pair);
        return component && component->GetWhiteBoxMesh() && component->GetActiveLayerId() == m_layerId;
    }

    void WhiteBoxExtrudeInsetWindow::RefreshSelection()
    {
        if (!HasCurrentLayer()) { Dismiss(); return; }
        TransformModelingLatch latch = TransformModelingLatch::None;
        EditorWhiteBoxTransformModeRequestBus::EventResult(latch, m_pair, &EditorWhiteBoxTransformModeRequests::GetModelingLatch);
        const QSignalBlocker latchBlock(m_latch);
        m_latch->setChecked(latch == (m_inset ? TransformModelingLatch::Inset : TransformModelingLatch::Extrude));
        // A typed amount only applies to polygons; edges extrude by dragging. Say which it is rather
        // than leaving a greyed-out button with no reason attached to it.
        const ModelingOps::Selection selection = ModelingOps::CurrentSelection(m_pair);
        if (!selection.m_polygons.empty())
        {
            m_selection->setText(tr("%1 polygons selected").arg(static_cast<int>(selection.m_polygons.size())));
        }
        else if (!m_inset && !selection.m_edges.empty())
        {
            m_selection->setText(tr("%1 edges selected - latch on and drag to extrude them")
                .arg(static_cast<int>(selection.m_edges.size())));
        }
        else
        {
            m_selection->setText(tr("Nothing selected"));
        }
        m_apply->setEnabled(ModelingOps::CanExtrudeInset(selection));
    }

    void WhiteBoxExtrudeInsetWindow::accept()
    {
        if (!HasCurrentLayer()) { Dismiss(); return; }
        const float amount = static_cast<float>(m_amount->value()) * (m_inset ? 0.01f : 1.0f);
        const auto result = ModelingOps::ExtrudeInset(m_pair, amount, m_inset);
        if (result.m_success) { Dismiss(); }
        else
        {
            m_status->setText(QString::fromUtf8(result.m_message.c_str()));
            m_status->show();
            RefreshSelection();
            adjustSize();
        }
    }

    void WhiteBoxExtrudeInsetWindow::reject()
    {
        Dismiss();
    }
}
