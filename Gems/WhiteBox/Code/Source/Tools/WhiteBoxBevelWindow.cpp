/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "Tools/WhiteBoxBevelWindow.h"
#include "EditorWhiteBoxComponent.h"
#include "Util/WhiteBoxEditorUtil.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>

namespace WhiteBox
{
    WhiteBoxBevelWindow::WhiteBoxBevelWindow(const AZ::EntityComponentIdPair& pair, QWidget* parent)
        : WhiteBoxModelingWindow(tr("Bevel"), parent)
        , m_pair(pair)
    {
        setObjectName("WhiteBoxBevelWindow");

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(1, 1, 1, 10);
        layout->setSpacing(10);
        auto* header = new QWidget(this);
        header->setObjectName("BevelHeader");
        SetDragHandle(header);
        auto* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(12, 9, 12, 9);
        auto* title = new QLabel(tr("Bevel"), header);
        title->setObjectName("BevelTitle");
        auto* live = new QLabel(tr("live"), header);
        live->setObjectName("BevelLive");
        headerLayout->addWidget(title);
        headerLayout->addStretch();
        headerLayout->addWidget(live);
        layout->addWidget(header);

        auto* controls = new QGridLayout();
        controls->setContentsMargins(12, 0, 12, 0);
        controls->setHorizontalSpacing(9);
        controls->setVerticalSpacing(7);
        m_width = new QDoubleSpinBox(this);
        m_width->setDecimals(3);
        m_width->setRange(0.001, 10000.0);
        m_width->setSingleStep(0.01);
        m_segments = new QSpinBox(this);
        m_segments->setRange(1, 32);
        m_profile = new QDoubleSpinBox(this);
        m_profile->setDecimals(2);
        m_profile->setRange(0.05, 0.95);
        m_profile->setSingleStep(0.05);
        // Apply typed values when complete; slider and arrow changes remain live.
        for (auto* spin : {static_cast<QAbstractSpinBox*>(m_width),
                          static_cast<QAbstractSpinBox*>(m_segments),
                          static_cast<QAbstractSpinBox*>(m_profile)})
        {
            spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
            spin->setKeyboardTracking(false);
            spin->setFixedWidth(65);
        }
        m_widthSlider = new QSlider(Qt::Horizontal, this);
        m_widthSlider->setRange(1, 1000);
        m_segmentsSlider = new QSlider(Qt::Horizontal, this);
        m_segmentsSlider->setRange(1, 32);
        m_profileSlider = new QSlider(Qt::Horizontal, this);
        m_profileSlider->setRange(5, 95);
        controls->addWidget(new QLabel(tr("Width"), this), 0, 0);
        controls->addWidget(m_width, 0, 1);
        controls->addWidget(m_widthSlider, 0, 2);
        controls->addWidget(new QLabel(tr("Segments"), this), 1, 0);
        controls->addWidget(m_segments, 1, 1);
        controls->addWidget(m_segmentsSlider, 1, 2);
        controls->addWidget(new QLabel(tr("Profile"), this), 2, 0);
        controls->addWidget(m_profile, 2, 1);
        controls->addWidget(m_profileSlider, 2, 2);
        controls->setColumnStretch(2, 1);
        layout->addLayout(controls);
        m_status = new QLabel(this);
        m_status->setObjectName("BevelError");
        m_status->setWordWrap(true);
        m_status->setContentsMargins(12, 0, 12, 0);
        m_status->hide();
        layout->addWidget(m_status);

        auto* actions = new QHBoxLayout();
        actions->setContentsMargins(12, 0, 12, 0);
        auto* bake = new QPushButton(tr("Bake  [Enter]"), this);
        bake->setObjectName("BevelBake");
        bake->setDefault(true);
        auto* cancel = new QPushButton(tr("Cancel  [Esc]"), this);
        cancel->setAutoDefault(false);
        actions->addWidget(bake);
        actions->addWidget(cancel);
        layout->addLayout(actions);

        if (auto* component = FindWhiteBoxComponent(m_pair))
        {
            m_layerId = component->GetActiveLayerId();
            m_widthRange = qMax(1.0, double(component->GetBevelParams().m_width) * 4.0);
        }
        RefreshValues(true);
        connect(m_width, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) { UpdateBevel(); });
        connect(m_segments, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { UpdateBevel(); });
        connect(m_profile, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) { UpdateBevel(); });
        connect(m_widthSlider, &QSlider::valueChanged, this, [this](int value)
        {
            m_width->setValue(qMax(0.001, m_widthRange * value / 1000.0));
        });
        connect(m_segmentsSlider, &QSlider::valueChanged, m_segments, &QSpinBox::setValue);
        connect(m_profileSlider, &QSlider::valueChanged, this, [this](int value) { m_profile->setValue(value / 100.0); });
        for (auto* spin : {static_cast<QAbstractSpinBox*>(m_width),
                          static_cast<QAbstractSpinBox*>(m_segments),
                          static_cast<QAbstractSpinBox*>(m_profile)})
        {
            connect(spin, &QAbstractSpinBox::editingFinished, this, [this]() { FinishEdit(); });
        }
        for (auto* slider : {m_widthSlider, m_segmentsSlider, m_profileSlider})
        {
            connect(slider, &QSlider::sliderReleased, this, [this]() { FinishEdit(); });
        }
        connect(bake, &QPushButton::clicked, this, &WhiteBoxBevelWindow::accept);
        connect(cancel, &QPushButton::clicked, this, &WhiteBoxBevelWindow::reject);
        auto* refresh = new QTimer(this);
        connect(refresh, &QTimer::timeout, this, [this]() { RefreshValues(); });
        refresh->start(150);
    }

    EditorWhiteBoxComponent* WhiteBoxBevelWindow::CurrentComponent() const
    {
        auto* component = FindWhiteBoxComponent(m_pair);
        return component && component->GetActiveLayerId() == m_layerId && component->HasActiveBevel()
            ? component : nullptr;
    }

    bool WhiteBoxBevelWindow::HasCurrentBevel() const
    {
        return CurrentComponent() != nullptr;
    }

    void WhiteBoxBevelWindow::RefreshValues(bool force)
    {
        auto* component = CurrentComponent();
        if (!component)
        {
            Dismiss();
            return;
        }
        // Do not overwrite partially typed text or move a slider under the mouse.
        if (!force && (m_width->hasFocus() || m_segments->hasFocus() || m_profile->hasFocus() ||
            m_widthSlider->isSliderDown() || m_segmentsSlider->isSliderDown() || m_profileSlider->isSliderDown()))
        {
            return;
        }
        const auto params = component->GetBevelParams();
        const QSignalBlocker widthBlock(m_width), segmentsBlock(m_segments), profileBlock(m_profile);
        const QSignalBlocker widthSliderBlock(m_widthSlider), segmentsSliderBlock(m_segmentsSlider),
            profileSliderBlock(m_profileSlider);
        m_width->setValue(params.m_width);
        m_segments->setValue(params.m_segments);
        m_profile->setValue(params.m_profile);
        if (params.m_width > m_widthRange)
        {
            m_widthRange = qMin(10000.0, double(params.m_width) * 2.0);
        }
        m_widthSlider->setValue(int(std::lround(params.m_width / m_widthRange * 1000.0)));
        m_segmentsSlider->setValue(params.m_segments);
        m_profileSlider->setValue(int(std::lround(params.m_profile * 100.0)));
    }

    void WhiteBoxBevelWindow::UpdateBevel()
    {
        auto* component = CurrentComponent();
        if (!component) { Dismiss(); return; }
        const EditorWhiteBoxComponent::BevelParams params{
            float(m_width->value()), m_segments->value(), float(m_profile->value())};
        AZStd::string error;
        AzToolsFramework::ScopedUndoBatch undo("White Box Bevel Parameters");
        if (component->SetParametricBevel({}, params, error))
        {
            component->SetBevelParams(params);
            undo.MarkEntityDirty(m_pair.GetEntityId());
            m_dirty = true;
            m_status->hide();
        }
        else
        {
            m_status->setText(QString::fromUtf8(error.c_str()));
            m_status->show();
        }
        RefreshValues(true); // Failed changes keep the last valid mesh and values.
        adjustSize();
    }

    void WhiteBoxBevelWindow::FinishEdit()
    {
        if (auto* component = CurrentComponent(); component && m_dirty)
        {
            AzToolsFramework::ScopedUndoBatch undo("White Box Commit Bevel Parameters");
            component->RebuildWhiteBox();
            undo.MarkEntityDirty(m_pair.GetEntityId());
            m_dirty = false;
        }
    }

    void WhiteBoxBevelWindow::accept()
    {
        if (auto* component = CurrentComponent())
        {
            AzToolsFramework::ScopedUndoBatch undo("White Box Bake Bevel");
            component->BakeBevel();
            undo.MarkEntityDirty(m_pair.GetEntityId());
        }
        Dismiss();
    }

    void WhiteBoxBevelWindow::reject()
    {
        if (auto* component = CurrentComponent())
        {
            AzToolsFramework::ScopedUndoBatch undo("White Box Cancel Bevel");
            component->CancelBevel();
            undo.MarkEntityDirty(m_pair.GetEntityId());
        }
        Dismiss();
    }

}
