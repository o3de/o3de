/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "RenderPlugin.h"
#include "../EMStudioCore.h"
#include <EMotionFX/CommandSystem/Source/ImporterCommands.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>


namespace EMStudio
{
    //-----------------------------------------------------------------------------------------
    // command callbacks
    //-----------------------------------------------------------------------------------------

    bool ReInitOpenGLRenderPlugin(bool clearActors = false, bool resetViewCloseup = false)
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(static_cast<uint32>(RenderPlugin::CLASS_ID));
        if (plugin == nullptr)
        {
            MCore::LogError("Cannot execute command callback. OpenGL render plugin does not exist.");
            return false;
        }

        RenderPlugin* renderPlugin = (RenderPlugin*)plugin;
        if (clearActors)
        {
            renderPlugin->CleanEMStudioActors();
        }

        renderPlugin->ReInit(resetViewCloseup);

        return true;
    }


    bool SelectionChangedRenderPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(static_cast<uint32>(RenderPlugin::CLASS_ID));
        if (plugin == nullptr)
        {
            MCore::LogError("Cannot execute command callback. OpenGL render plugin does not exist.");
            return false;
        }

        RenderPlugin* renderPlugin = (RenderPlugin*)plugin;
        renderPlugin->ReInitTransformationManipulators();
        renderPlugin->SetSkipFollowCalcs(true);

        return true;
    }


    bool ResetTrajectoryPathRenderPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(static_cast<uint32>(RenderPlugin::CLASS_ID));
        if (plugin == nullptr)
        {
            MCore::LogError("Cannot execute command callback. OpenGL render plugin does not exist.");
            return false;
        }

        RenderPlugin* renderPlugin = (RenderPlugin*)plugin;
        renderPlugin->ResetSelectedTrajectoryPaths();

        return true;
    }


    bool AdjustActorInstanceRenderPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(static_cast<uint32>(RenderPlugin::CLASS_ID));
        if (plugin == nullptr)
        {
            MCore::LogError("Cannot execute command callback. OpenGL render plugin does not exist.");
            return false;
        }

        RenderPlugin* renderPlugin = (RenderPlugin*)plugin;
        renderPlugin->SetSkipFollowCalcs(true);

        return true;
    }

    bool RenderPlugin::UpdateRenderActorsCallback::Execute([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)      { return ReInitOpenGLRenderPlugin(); }
    bool RenderPlugin::UpdateRenderActorsCallback::Undo([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)         { return ReInitOpenGLRenderPlugin(); }
    bool RenderPlugin::ReInitRenderActorsCallback::Execute([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)      { return ReInitOpenGLRenderPlugin(true, commandLine.GetValueAsBool("resetViewCloseup", true)); }
    bool RenderPlugin::ReInitRenderActorsCallback::Undo([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)         { return ReInitOpenGLRenderPlugin(true, commandLine.GetValueAsBool("resetViewCloseup", true)); }

    bool RenderPlugin::CreateActorInstanceCallback::Execute([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)     { return ReInitOpenGLRenderPlugin(/*clearActors=*/false, /*resetViewCloseup=*/true); }
    bool RenderPlugin::CreateActorInstanceCallback::Undo([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)        { return ReInitOpenGLRenderPlugin(); }
    bool RenderPlugin::RemoveActorInstanceCallback::Execute([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)     { return ReInitOpenGLRenderPlugin(); }
    bool RenderPlugin::RemoveActorInstanceCallback::Undo([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)        { return ReInitOpenGLRenderPlugin(); }

    bool RenderPlugin::SelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return SelectionChangedRenderPlugin();
    }
    bool RenderPlugin::SelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return SelectionChangedRenderPlugin();
    }
    bool RenderPlugin::UnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return SelectionChangedRenderPlugin();
    }
    bool RenderPlugin::UnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return SelectionChangedRenderPlugin();
    }
    bool RenderPlugin::ClearSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return SelectionChangedRenderPlugin();
    }
    bool RenderPlugin::ClearSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return SelectionChangedRenderPlugin();
    }

    bool RenderPlugin::CommandResetToBindPoseCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)  { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ResetTrajectoryPathRenderPlugin(); }
    bool RenderPlugin::CommandResetToBindPoseCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ResetTrajectoryPathRenderPlugin(); }

    bool RenderPlugin::AdjustActorInstanceCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return AdjustActorInstanceRenderPlugin(); }
    bool RenderPlugin::AdjustActorInstanceCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return AdjustActorInstanceRenderPlugin(); }
} // namespace EMStudio
