/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMStudio/AtomRenderPlugin.h>
#include <EMStudio/AnimViewportRenderer.h>
#include <EMStudio/AnimViewportToolBar.h>

#include <Integration/Components/ActorComponent.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <QHBoxLayout>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(AtomRenderPlugin, EMotionFX::EditorAllocator, 0);

    AtomRenderPlugin::AtomRenderPlugin()
        : DockWidgetPlugin()
    {
    }

    AtomRenderPlugin::~AtomRenderPlugin()
    {
        GetCommandManager()->RemoveCommandCallback(m_importActorCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_removeActorCallback, false);
        delete m_importActorCallback;
        delete m_removeActorCallback;
    }

    const char* AtomRenderPlugin::GetName() const
    {
        return "Atom Render Window (Preview)";
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

    QWidget* AtomRenderPlugin::GetInnerWidget()
    {
        return m_innerWidget;
    }

    void AtomRenderPlugin::ReinitRenderer()
    {
        m_animViewportWidget->Reinit();
    }

    bool AtomRenderPlugin::Init()
    {
        LoadRenderOptions();

        m_innerWidget = new QWidget();
        m_dock->setWidget(m_innerWidget);

        QVBoxLayout* verticalLayout = new QVBoxLayout(m_innerWidget);
        verticalLayout->setSizeConstraint(QLayout::SetNoConstraint);
        verticalLayout->setSpacing(1);
        verticalLayout->setMargin(0);

        // Add the viewport widget
        m_animViewportWidget = new AnimViewportWidget(this);

        // Add the tool bar
        AnimViewportToolBar* toolBar = new AnimViewportToolBar(m_innerWidget);
        toolBar->SetRenderFlags(m_animViewportWidget->GetRenderFlags());

        verticalLayout->addWidget(toolBar);
        verticalLayout->addWidget(m_animViewportWidget);

        // Register command callbacks.
        m_importActorCallback = new ImportActorCallback(false);
        m_removeActorCallback = new RemoveActorCallback(false);
        EMStudioManager::GetInstance()->GetCommandManager()->RegisterCommandCallback("ImportActor", m_importActorCallback);
        EMStudioManager::GetInstance()->GetCommandManager()->RegisterCommandCallback("RemoveActor", m_removeActorCallback);

        return true;
    }

    void AtomRenderPlugin::LoadRenderOptions()
    {
        AZStd::string renderOptionsFilename(GetManager()->GetAppDataFolder());
        renderOptionsFilename += "EMStudioRenderOptions.cfg";
        QSettings settings(renderOptionsFilename.c_str(), QSettings::IniFormat, this);
        m_renderOptions = RenderOptions::Load(&settings);
    }

    const RenderOptions* AtomRenderPlugin::GetRenderOptions() const
    {
        return &m_renderOptions;
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

    bool AtomRenderPlugin::ImportActorCallback::Execute(
        [[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        return ReinitAtomRenderPlugin();
    }
    bool AtomRenderPlugin::ImportActorCallback::Undo(
        [[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        return ReinitAtomRenderPlugin();
    }

    bool AtomRenderPlugin::RemoveActorCallback::Execute(
        [[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        return ReinitAtomRenderPlugin();
    }
    bool AtomRenderPlugin::RemoveActorCallback::Undo(
        [[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        return ReinitAtomRenderPlugin();
    }
}// namespace EMStudio
