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


    bool RenderPlugin::UpdateRenderActorsCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)      { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitOpenGLRenderPlugin(); }
    bool RenderPlugin::UpdateRenderActorsCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)         { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitOpenGLRenderPlugin(); }
    bool RenderPlugin::ReInitRenderActorsCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)      { MCORE_UNUSED(command); return ReInitOpenGLRenderPlugin(true, commandLine.GetValueAsBool("resetViewCloseup", true)); }
    bool RenderPlugin::ReInitRenderActorsCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)         { MCORE_UNUSED(command); return ReInitOpenGLRenderPlugin(true, commandLine.GetValueAsBool("resetViewCloseup", true)); }

    bool RenderPlugin::CreateActorInstanceCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitOpenGLRenderPlugin(); }
    bool RenderPlugin::CreateActorInstanceCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitOpenGLRenderPlugin(); }
    bool RenderPlugin::RemoveActorInstanceCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitOpenGLRenderPlugin(); }
    bool RenderPlugin::RemoveActorInstanceCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitOpenGLRenderPlugin(); }

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
