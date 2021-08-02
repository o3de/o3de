/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "TrackViewDoubleSpinBox.h"

CTrackViewDoubleSpinBox::CTrackViewDoubleSpinBox(QWidget* parent) 
    : AzQtComponents::DoubleSpinBox(parent)
{
}

CTrackViewDoubleSpinBox::~CTrackViewDoubleSpinBox()
{
}

void CTrackViewDoubleSpinBox::stepBy(int steps)
{
    AzQtComponents::DoubleSpinBox::stepBy(steps);
    Q_EMIT stepByFinished();
}

#include <TrackView/moc_TrackViewDoubleSpinBox.cpp>
