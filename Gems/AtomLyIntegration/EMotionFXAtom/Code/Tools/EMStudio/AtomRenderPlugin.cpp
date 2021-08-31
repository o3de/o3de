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

    const char* AtomRenderPlugin::GetName() const
    {
        return "Atom Render Window";
    }

    uint32 AtomRenderPlugin::GetClassID() const
    {
        return static_cast<uint32>(AtomRenderPlugin::CLASS_ID);
    }

    const char* AtomRenderPlugin::GetCreatorName() const
    {
        return "O3DE";
    }

    float AtomRenderPlugin::GetVersion() const
    {
        return 1.0f;
    }

    bool AtomRenderPlugin::GetIsClosable() const
    {
        return true;
    }

    bool AtomRenderPlugin::GetIsFloatable() const
    {
        return true;
    }

    bool AtomRenderPlugin::GetIsVertical() const
    {
        return false;
    }

    EMStudioPlugin* AtomRenderPlugin::Clone()
    {
        return new AtomRenderPlugin();
    }

    EMStudioPlugin::EPluginType AtomRenderPlugin::GetPluginType() const
    {
        return EMStudioPlugin::PLUGINTYPE_RENDERING;
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
