/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMStudio/AtomRenderPlugin.h>
#include <QHBoxLayout>

namespace EMStudio
{
    AtomRenderPlugin::AtomRenderPlugin()
        : DockWidgetPlugin()
    {
    }

    AtomRenderPlugin::~AtomRenderPlugin()
    {

    }

    bool AtomRenderPlugin::Init()
    {
        m_innerWidget = new QWidget();
        m_dock->setWidget(m_innerWidget);

        QVBoxLayout* verticalLayout = new QVBoxLayout(m_innerWidget);
        verticalLayout->setSizeConstraint(QLayout::SetNoConstraint);
        verticalLayout->setSpacing(1);
        verticalLayout->setMargin(0);

        m_animViewportWidget = new AnimViewportWidget(m_innerWidget);

        return true;
    }
}
