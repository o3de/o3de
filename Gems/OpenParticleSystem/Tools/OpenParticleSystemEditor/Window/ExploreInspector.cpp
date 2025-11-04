/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ExploreInspector.h>

#include <QVBoxLayout>
#include <QScrollArea>
#include <QAction>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMessageBox>


namespace OpenParticleSystemEditor
{
    ExploreInspector::ExploreInspector(QWidget* parent)
        : QWidget(parent)
    {
        m_layout = new QVBoxLayout();
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setSpacing(0);

        setLayout(m_layout);
    }

    ExploreInspector::~ExploreInspector()
    {
    }
} // namespace OpenParticleSystemEditor

#include <moc_ExploreInspector.cpp>
