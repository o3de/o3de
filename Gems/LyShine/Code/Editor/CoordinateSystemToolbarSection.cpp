/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include <LyShine/Bus/UiEditorCanvasBus.h>
#include "CanvasHelpers.h"

#include <QComboBox>
#include <QCheckBox>

namespace
{
    void SetCoordinateSystemFromCombobox(EditorWindow* editorWindow, QComboBox* combobox, int newIndex)
    {
        ViewportInteraction::CoordinateSystem s = (ViewportInteraction::CoordinateSystem)combobox->itemData(newIndex).toInt();

        editorWindow->GetViewport()->GetViewportInteraction()->SetCoordinateSystem(s);
    }
} // anonymous namespace.

CoordinateSystemToolbarSection::CoordinateSystemToolbarSection(QToolBar* parent, bool addSeparator)
    : QObject()
    , m_editorWindow(qobject_cast<EditorWindow*>(parent->parent()))
    , m_combobox(new QComboBox(parent))
    , m_snapCheckbox(new QCheckBox("Snap to grid", parent))
{
    AZ_Assert(m_editorWindow, "Invalid hierarchy of windows.");

    m_combobox->setToolTip(QString("Reference coordinate system (%1)").arg(UICANVASEDITOR_COORDINATE_SYSTEM_CYCLE_SHORTCUT_KEY_SEQUENCE.toString()));
    m_combobox->setMinimumContentsLength(6);

    // Combobox.
    {
        parent->addWidget(m_combobox);

        for (const auto s : ViewportInteraction::CoordinateSystem())
        {
            m_combobox->addItem(ViewportHelpers::CoordinateSystemToString((int)s), (int)s);
        }

        QObject::connect(m_combobox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), m_combobox,         // IMPORTANT: We HAVE to use static_cast<>() to specify which overload of QComboBox::currentIndexChanged() we want.
            [this](int index)
            {
                SetCoordinateSystemFromCombobox(m_editorWindow, m_combobox, index);
            });

        QObject::connect(m_editorWindow,
            SIGNAL(SignalCoordinateSystemCycle()),
            SLOT(HandleCoordinateSystemCycle()));
    }

    // Snap to grid checkbox.
    {
        parent->addSeparator();

        m_snapCheckbox->setToolTip(QString("Toggle snap to grid (%1)").arg(UICANVASEDITOR_SNAP_TO_GRID_TOGGLE_SHORTCUT_KEY_SEQUENCE.toString()));

        // IMPORTANT: The MainToolbar is creater BEFORE the canvas is loaded.
        // The checked state of m_snapCheckbox will be set by
        // SetSnapToGridIsChecked() after the canvas is loaded.

        QObject::connect(m_snapCheckbox,
            &QCheckBox::clicked, m_snapCheckbox,
            [this]([[maybe_unused]] bool checked)
            {
                UpdateCanvasSnapEnabled();
            });

        parent->addWidget(m_snapCheckbox);

        QObject::connect(m_editorWindow,
            SIGNAL(SignalSnapToGridToggle()),
            SLOT(HandleSnapToGridToggle()));
    }

    if (addSeparator)
    {
        parent->addSeparator();
    }
}

int CoordinateSystemToolbarSection::CycleSelectedItem()
{
    int newIndex = (m_combobox->currentIndex() + 1) % m_combobox->count();

    m_combobox->setCurrentIndex(newIndex);

    return newIndex;
}

void CoordinateSystemToolbarSection::SetIsEnabled(bool enabled)
{
    m_combobox->setEnabled(enabled);
}

void CoordinateSystemToolbarSection::SetCurrentIndex(int index)
{
    AZ_Assert(index < m_combobox->count(), "Invalid index.");
    m_combobox->setCurrentIndex(index);
}

void CoordinateSystemToolbarSection::HandleCoordinateSystemCycle()
{
    if (m_combobox->isEnabled())
    {
        SetCoordinateSystemFromCombobox(m_editorWindow, m_combobox, CycleSelectedItem());
    }
}

void CoordinateSystemToolbarSection::HandleSnapToGridToggle()
{
    if (m_snapCheckbox->isEnabled())
    {
        m_snapCheckbox->toggle();

        UpdateCanvasSnapEnabled();
    }
}

void CoordinateSystemToolbarSection::SetSnapToGridIsChecked(bool checked)
{
    m_snapCheckbox->setChecked(checked);
}

void CoordinateSystemToolbarSection::UpdateCanvasSnapEnabled()
{
    bool checked = (m_snapCheckbox->checkState() == Qt::Checked);

    // Add an undo command
    AZStd::string canvasUndoXml = CanvasHelpers::BeginUndoableCanvasChange(m_editorWindow->GetCanvas());
    UiEditorCanvasBus::Event(m_editorWindow->GetCanvas(), &UiEditorCanvasBus::Events::SetIsSnapEnabled, checked);
    CanvasHelpers::EndUndoableCanvasChange(m_editorWindow, "toggle snapping", canvasUndoXml);
}

#include <moc_CoordinateSystemToolbarSection.cpp>
