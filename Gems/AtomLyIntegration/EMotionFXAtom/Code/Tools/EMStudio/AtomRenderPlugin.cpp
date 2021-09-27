/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMStudio/AtomRenderPlugin.h>
#include <EMStudio/AnimViewportRenderer.h>

#include <Integration/Components/ActorComponent.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
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

    void AtomRenderPlugin::ReinitRenderer()
    {
        m_animViewportWidget->GetAnimViewportRenderer()->Reinit();
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

        // Register command callbacks.
        m_createActorInstanceCallback = new CreateActorInstanceCallback(false);
        EMStudioManager::GetInstance()->GetCommandManager()->RegisterCommandCallback("CreateActorInstance", m_createActorInstanceCallback);

        return true;
    }

    // Command callbacks
    bool ReinitAtomRenderPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(static_cast<uint32>(AtomRenderPlugin::CLASS_ID));
        if (!plugin)
        {
            AZ_Error("AtomRenderPlugin", false, "Cannot execute command callback. Atom render plugin does not exist.");
            return false;
        }

        AtomRenderPlugin* atomRenderPlugin = static_cast<AtomRenderPlugin*>(plugin);
        atomRenderPlugin->ReinitRenderer();

        return true;
    }

    bool AtomRenderPlugin::CreateActorInstanceCallback::Execute(
        [[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        return ReinitAtomRenderPlugin();
    }
    bool AtomRenderPlugin::CreateActorInstanceCallback::Undo(
        [[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        return ReinitAtomRenderPlugin();
    }

}
