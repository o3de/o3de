/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "UiCanvasEditor_precompiled.h"

#include "EditorCommon.h"

#include <QLabel>

MainToolbar::MainToolbar(EditorWindow* parent)
    : QToolBar("Main Toolbar", parent)
    , m_newElementToolbarSection(new NewElementToolbarSection(this, true))
    , m_coordinateSystemToolbarSection(new CoordinateSystemToolbarSection(this, true))
    , m_canvasSizeToolbarSection(new ReferenceCanvasSizeToolbarSection(this, false))
    , m_zoomFactorSpinBox(new AzQtComponents::DoubleSpinBox(parent))
{
    setObjectName("MainToolbar");    // needed to save state
    setFloatable(false);

    // Zoom factor.
    QLabel* zoomLabel = new QLabel("Zoom:", parent);
    m_zoomFactorSpinBox->setRange(10.0, 1000.0);
    m_zoomFactorSpinBox->setSingleStep(20.0);
    m_zoomFactorSpinBox->setSuffix("%");
    m_zoomFactorSpinBox->setValue(100.0);
    m_zoomFactorSpinBox->setDecimals(2);
    m_zoomFactorSpinBox->setToolTip(QString("Canvas zoom percentage"));
    m_zoomFactorSpinBox->setKeyboardTracking(false);
    m_zoomFactorSpinBox->setButtonSymbols(QDoubleSpinBox::ButtonSymbols::UpDownArrows);

    QObject::connect(m_zoomFactorSpinBox,
        QOverload<double>::of(&AzQtComponents::DoubleSpinBox::valueChanged),
        [this, parent](double value)
        {
            parent->GetViewport()->GetViewportInteraction()->SetCanvasZoomPercent(static_cast<float>(value));
        });

    addWidget(zoomLabel);
    addWidget(m_zoomFactorSpinBox);

    parent->addToolBar(this);
}

void MainToolbar::SetZoomPercent(float zoomPercent)
{
    // when setting the zoom percentage from outside the spinbox we do not want the valueChanged signal to be sent
    m_zoomFactorSpinBox->blockSignals(true);
    m_zoomFactorSpinBox->setValue(zoomPercent);
    m_zoomFactorSpinBox->blockSignals(false);
}

#include <moc_MainToolbar.cpp>
