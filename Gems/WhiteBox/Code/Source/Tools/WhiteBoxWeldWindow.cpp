/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "Tools/WhiteBoxWeldWindow.h"
#include "EditorWhiteBoxComponent.h"
#include "Util/WhiteBoxEditorUtil.h"
#include "SubComponentModes/EditorWhiteBoxTransformModeBus.h"
#include "Util/WhiteBoxModelingOps.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace WhiteBox
{
    WhiteBoxWeldWindow::WhiteBoxWeldWindow(const AZ::EntityComponentIdPair& pair, QWidget* parent)
        : WhiteBoxModelingWindow(tr("Weld"), parent)
        , m_pair(pair)
    {
        setObjectName("WhiteBoxWeldWindow");
        if (auto* component = FindWhiteBoxComponent(m_pair))
        {
            m_layerId = component->GetActiveLayerId();
        }
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(12, 9, 12, 10);
        layout->setSpacing(10);
        auto* title = new QLabel(tr("Weld"), this);
        SetDragHandle(title);
        layout->addWidget(title);
        auto* row = new QHBoxLayout();
        row->addWidget(new QLabel(tr("Target"), this));
        m_target = new QComboBox(this);
        m_target->addItem(tr("Selection Center"), false);
        m_target->addItem(tr("Last Selected Vertex"), true);
        m_target->setToolTip(tr("Merge at the average position, or at the last vertex added to the selection."));
        row->addWidget(m_target, 1);
        layout->addLayout(row);
        m_selection = new QLabel(this);
        layout->addWidget(m_selection);
        m_status = new QLabel(this);
        m_status->setWordWrap(true);
        m_status->hide();
        layout->addWidget(m_status);
        auto* actions = new QHBoxLayout();
        m_weld = new QPushButton(tr("Weld  [Enter]"), this);
        m_weld->setDefault(true);
        auto* cancel = new QPushButton(tr("Cancel  [Esc]"), this);
        cancel->setAutoDefault(false);
        actions->addWidget(m_weld);
        actions->addWidget(cancel);
        layout->addLayout(actions);
        connect(m_weld, &QPushButton::clicked, this, &WhiteBoxWeldWindow::accept);
        connect(cancel, &QPushButton::clicked, this, &WhiteBoxWeldWindow::reject);
        connect(m_target, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int)
        {
            m_status->hide();
            adjustSize();
        });
        auto* refresh = new QTimer(this);
        connect(refresh, &QTimer::timeout, this, [this]() { RefreshSelection(); });
        refresh->start(150);
        RefreshSelection();
    }

    bool WhiteBoxWeldWindow::HasCurrentLayer() const
    {
        auto* component = FindWhiteBoxComponent(m_pair);
        return component && component->GetWhiteBoxMesh() && component->GetActiveLayerId() == m_layerId;
    }

    void WhiteBoxWeldWindow::RefreshSelection()
    {
        if (!HasCurrentLayer())
        {
            Dismiss();
            return;
        }
        Api::VertexHandles vertices;
        EditorWhiteBoxTransformModeRequestBus::EventResult(
            vertices, m_pair, &EditorWhiteBoxTransformModeRequests::GetSelectedVertices);
        m_selection->setText(tr("%1 vertices selected").arg(static_cast<int>(vertices.size())));
        m_weld->setEnabled(vertices.size() >= 2);
    }

    void WhiteBoxWeldWindow::accept()
    {
        // Revalidate at the click, not just on the refresh timer, so switching layers is safe.
        if (!HasCurrentLayer()) { Dismiss(); return; }
        const auto result = ModelingOps::Weld(m_pair, m_target->currentData().toBool());
        if (result.m_success)
        {
            Dismiss();
        }
        else
        {
            m_status->setText(QString::fromUtf8(result.m_message.c_str()));
            m_status->show();
            RefreshSelection();
            adjustSize();
        }
    }

    void WhiteBoxWeldWindow::reject()
    {
        Dismiss();
    }
}
