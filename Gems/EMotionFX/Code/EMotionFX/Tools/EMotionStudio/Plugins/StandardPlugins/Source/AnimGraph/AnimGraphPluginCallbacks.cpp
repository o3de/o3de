/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "AnimGraphPlugin.h"
#include "AnimGraphModel.h"
#include "BlendGraphWidget.h"
#include "NodeGraph.h"
#include "NodePaletteWidget.h"
#include "BlendGraphViewWidget.h"
#include "GraphNode.h"
#include "NavigateWidget.h"
#include "AttributesWindow.h"
#include "BlendTreeVisualNode.h"
#include "StateGraphNode.h"
#include "ParameterWindow.h"
#include "GraphNodeFactory.h"
#include <QDockWidget>
#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <MCore/Source/LogManager.h>

#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include <MysticQt/Source/DialogStack.h>

#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>

#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <EMotionFX/CommandSystem/Source/MiscCommands.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/Source/AnimGraphBus.h>
#include <EMotionFX/Source/MotionManager.h>
#include <Editor/AnimGraphEditorBus.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewToolBar.h>


namespace EMStudio
{
    bool AnimGraphPlugin::CommandActivateAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine) 
    { 
        MCORE_UNUSED(command); 
        MCORE_UNUSED(commandLine); 
        
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            return false;
        }
        animGraphPlugin->GetParameterWindow()->Reinit();
        
        return true; 
    }

    bool AnimGraphPlugin::CommandActivateAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine) 
    { 
        MCORE_UNUSED(command); 
        MCORE_UNUSED(commandLine); 
        return true;
    }
    
    bool SetFirstSelectedAnimGraphActive()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        EMotionFX::AnimGraph* firstSelectedAnimGraph = CommandSystem::GetCommandManager()->GetCurrentSelection().GetFirstAnimGraph();
        animGraphPlugin->SetActiveAnimGraph(firstSelectedAnimGraph);
        return true;
    }


    bool AnimGraphPlugin::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false)
        {
            EMotionFX::AnimGraphEditorRequestBus::Broadcast(&EMotionFX::AnimGraphEditorRequests::UpdateMotionSetComboBox);
            return true;
        }
        const bool result = SetFirstSelectedAnimGraphActive();
        EMotionFX::AnimGraphEditorRequestBus::Broadcast(&EMotionFX::AnimGraphEditorRequests::UpdateMotionSetComboBox);
        return result;
    }
    bool AnimGraphPlugin::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        EMotionFX::AnimGraphEditorRequestBus::Broadcast(&EMotionFX::AnimGraphEditorRequests::UpdateMotionSetComboBox);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false)
        {
            EMotionFX::AnimGraphEditorRequestBus::Broadcast(&EMotionFX::AnimGraphEditorRequests::UpdateMotionSetComboBox);
            return true;
        }
        const bool result = SetFirstSelectedAnimGraphActive();
        EMotionFX::AnimGraphEditorRequestBus::Broadcast(&EMotionFX::AnimGraphEditorRequests::UpdateMotionSetComboBox);
        return result;
    }
    bool AnimGraphPlugin::CommandUnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return SetFirstSelectedAnimGraphActive();
    }
    bool AnimGraphPlugin::CommandUnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return SetFirstSelectedAnimGraphActive();
    }
    bool AnimGraphPlugin::CommandClearSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)   
    { 
        MCORE_UNUSED(command); 
        MCORE_UNUSED(commandLine); 
        return SetFirstSelectedAnimGraphActive(); 
    }

    bool AnimGraphPlugin::CommandClearSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)      
    { 
        MCORE_UNUSED(command); 
        MCORE_UNUSED(commandLine); 
        return SetFirstSelectedAnimGraphActive(); 
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    // CommandRecorderClearCallback callback
    //----------------------------------------------------------------------------------------------------------------------------------
    bool AnimGraphPlugin::CommandRecorderClearCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(commandLine);

        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (!plugin)
        {
            return false;
        }

        CommandSystem::CommandRecorderClear* clearRecorderCommand = static_cast<CommandSystem::CommandRecorderClear*>(command);
        if (clearRecorderCommand->m_wasRecording || clearRecorderCommand->m_wasInPlayMode)
        {
            AnimGraphPlugin* animGraphPlugin = static_cast<AnimGraphPlugin*>(plugin);
            EMStudioPlugin* timeViewPlugin = EMStudio::GetPluginManager()->FindActivePlugin(TimeViewPlugin::CLASS_ID);
            if (timeViewPlugin)
            {
                static_cast<TimeViewPlugin*>(timeViewPlugin)->GetTimeViewToolBar()->OnClearRecordButton();
            }
            animGraphPlugin->GetParameterWindow()->Reinit();
        }

        return true;
    }


    bool AnimGraphPlugin::CommandRecorderClearCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return true;
    }


    bool AnimGraphPlugin::CommandPlayMotionCallback::Execute([[maybe_unused]] MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(commandLine);

        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (!plugin)
        {
            return false;
        }

        AnimGraphPlugin* animGraphPlugin = static_cast<AnimGraphPlugin*>(plugin);
        animGraphPlugin->GetParameterWindow()->Reinit();

        return true;
    }


    bool AnimGraphPlugin::CommandPlayMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return true;
    }
} // namespace EMStudio
