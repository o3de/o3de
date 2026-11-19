/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Tools/WhiteBoxSmoothingWindow.h"
#include "EditorWhiteBoxComponent.h"
#include "SubComponentModes/EditorWhiteBoxTransformModeBus.h"
#include "Util/WhiteBoxEditorUtil.h"
#include "Util/WhiteBoxModelingOps.h"

#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace WhiteBox
{
    WhiteBoxSmoothingWindow::WhiteBoxSmoothingWindow(const AZ::EntityComponentIdPair& pair, QWidget* parent)
        : WhiteBoxModelingWindow(tr("Smoothing Groups"), parent)
        , m_pair(pair)
    {
        setObjectName("WhiteBoxSmoothingWindow");
        if (auto* component = FindWhiteBoxComponent(m_pair))
        {
            m_layerId = component->GetActiveLayerId();
        }

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(12, 9, 12, 10);
        layout->setSpacing(10);
        auto* title = new QLabel(tr("Smoothing Groups"), this);
        SetDragHandle(title);
        layout->addWidget(title);

        auto* grid = new QGridLayout();
        grid->setHorizontalSpacing(3);
        grid->setVerticalSpacing(3);
        for (int group = 0; group < GroupCount; ++group)
        {
            auto* button = new QPushButton(QString::number(group + 1), this);
            button->setCheckable(true);
            button->setAutoDefault(false);
            button->setFixedSize(30, 24);
            button->setToolTip(tr("Faces sharing group %1 smooth across their shared vertices. Click to add or remove it.").arg(group + 1));
            grid->addWidget(button, group / 8, group % 8);
            connect(button, &QPushButton::clicked, this, [this, group](bool) { ToggleGroup(group); });
            m_groups[group] = button;
        }
        layout->addLayout(grid);

        auto* autoRow = new QHBoxLayout();
        m_autoSmooth = new QPushButton(tr("Auto Smooth"), this);
        m_autoSmooth->setToolTip(
            tr("Group the selection (or the whole layer with nothing selected) so edges sharper than the angle stay hard."));
        m_angle = new QDoubleSpinBox(this);
        m_angle->setRange(0.0, 180.0);
        m_angle->setSingleStep(5.0);
        m_angle->setDecimals(1);
        m_angle->setValue(30.0);
        m_angle->setSuffix(tr(" deg"));
        m_angle->setToolTip(tr("Neighbours meeting at less than this angle smooth together."));
        autoRow->addWidget(m_autoSmooth);
        autoRow->addWidget(m_angle);
        layout->addLayout(autoRow);

        m_selection = new QLabel(this);
        layout->addWidget(m_selection);
        m_status = new QLabel(this);
        m_status->setWordWrap(true);
        m_status->hide();
        layout->addWidget(m_status);

        auto* actions = new QHBoxLayout();
        m_clear = new QPushButton(tr("Clear (Flat)"), this);
        m_clear->setToolTip(tr("Remove every group from the selected polygons so they shade flat."));
        auto* close = new QPushButton(tr("Close  [Esc]"), this);
        for (auto* button : { m_autoSmooth, m_clear, close })
        {
            button->setAutoDefault(false);
            button->setDefault(false);
        }
        actions->addWidget(m_clear);
        actions->addWidget(close);
        layout->addLayout(actions);

        connect(m_clear, &QPushButton::clicked, this, [this]()
        {
            const auto result = ModelingOps::EditSmoothingGroups(m_pair, 0, Api::SmoothingEdit::Set);
            ShowResult(result.m_success, result.m_message);
            RefreshSelection();
        });
        connect(m_autoSmooth, &QPushButton::clicked, this, [this]()
        {
            if (!HasCurrentLayer())
            {
                Dismiss();
                return;
            }
            const auto result = ModelingOps::AutoSmooth(m_pair, static_cast<float>(m_angle->value()));
            ShowResult(result.m_success, result.m_message);
            RefreshSelection();
        });
        connect(close, &QPushButton::clicked, this, &WhiteBoxSmoothingWindow::reject);

        auto* refresh = new QTimer(this);
        connect(refresh, &QTimer::timeout, this, [this]() { RefreshSelection(); });
        refresh->start(150);
        RefreshSelection();
    }

    bool WhiteBoxSmoothingWindow::HasCurrentLayer() const
    {
        auto* component = FindWhiteBoxComponent(m_pair);
        return component && component->GetWhiteBoxMesh() && component->GetActiveLayerId() == m_layerId;
    }

    void WhiteBoxSmoothingWindow::RefreshSelection()
    {
        if (!HasCurrentLayer())
        {
            Dismiss();
            return;
        }
        Api::PolygonHandles polygons;
        EditorWhiteBoxTransformModeRequestBus::EventResult(
            polygons, m_pair, &EditorWhiteBoxTransformModeRequests::GetSelectedPolygons);
        m_selection->setText(polygons.empty() ? tr("Select polygons in Transform mode, or Auto Smooth the whole layer.")
                                              : tr("%n polygon(s) selected", nullptr, static_cast<int>(polygons.size())));
        const auto state = ModelingOps::SelectedSmoothingGroups(m_pair);
        const bool usable = state.has_value();
        m_clear->setEnabled(usable);
        const AZ::u32 all = usable ? state->m_all : 0;
        const AZ::u32 any = usable ? state->m_any : 0;
        if (all == m_shownAll && any == m_shownAny && m_groups[0]->isEnabled() == usable)
        {
            return;
        }
        m_shownAll = all;
        m_shownAny = any;
        for (int group = 0; group < GroupCount; ++group)
        {
            const AZ::u32 bit = 1u << group;
            QPushButton* button = m_groups[group];
            button->setEnabled(usable);
            button->setChecked((all & bit) != 0);
            // Only some of the selection has this group: outline it rather than light it.
            button->setStyleSheet((all & bit) == 0 && (any & bit) != 0 ? QStringLiteral("border: 1px dashed #67c7e8;") : QString());
        }
    }

    void WhiteBoxSmoothingWindow::ToggleGroup(const int group)
    {
        // Revalidate at the edit, not just on the timer, so switching layers is safe.
        if (!HasCurrentLayer())
        {
            Dismiss();
            return;
        }
        const AZ::u32 bit = 1u << group;
        const auto state = ModelingOps::SelectedSmoothingGroups(m_pair);
        const bool remove = state.has_value() && (state->m_all & bit) != 0;
        const auto result =
            ModelingOps::EditSmoothingGroups(m_pair, bit, remove ? Api::SmoothingEdit::Remove : Api::SmoothingEdit::Add);
        ShowResult(result.m_success, result.m_message);
        m_shownAll = m_shownAny = ~0u; // the click already toggled the button, so restyle from the mesh
        RefreshSelection();
    }

    void WhiteBoxSmoothingWindow::ShowResult(const bool success, const AZStd::string& message)
    {
        // Success is visible in the viewport; only a refusal needs words.
        m_status->setText(QString::fromUtf8(message.c_str()));
        m_status->setVisible(!success && !message.empty());
        adjustSize();
    }

    void WhiteBoxSmoothingWindow::reject()
    {
        Dismiss();
    }
} // namespace WhiteBox
