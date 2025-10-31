/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/Inspector/NoSelectionWidget.h>
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>

namespace EMStudio
{
    NoSelectionWidget::NoSelectionWidget(QWidget* parent)
        : QWidget(parent)
    {
        m_label = new QLabel("Select an object to show its properties in the inspector.", this);

        QFrame* line = new QFrame(this);
        line->setObjectName(QString::fromUtf8("line"));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignTop);
        layout->addWidget(m_label);
        layout->addWidget(line);
    }
} // namespace EMStudio
