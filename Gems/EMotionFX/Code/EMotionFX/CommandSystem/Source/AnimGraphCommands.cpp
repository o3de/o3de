/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/StringFunc/StringFunc.h>
#include "AnimGraphCommands.h"
#include "CommandManager.h"
#include <MCore/Source/FileSystem.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/ExporterFileProcessor.h>
#include <AzFramework/API/ApplicationAPI.h>


namespace CommandSystem
{
    const char* CommandActivateAnimGraph::s_activateAnimGraphCmdName = "ActivateAnimGraph";
    //-------------------------------------------------------------------------------------
    // Load the given anim graph
    //-------------------------------------------------------------------------------------

    // constructor
    CommandLoadAnimGraph::CommandLoadAnimGraph(MCore::Command* orgCommand)
        : MCore::Command("LoadAnimGraph", orgCommand)
    {
        m_oldAnimGraphId = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandLoadAnimGraph::~CommandLoadAnimGraph()
    {
    }

    // execute
    bool CommandLoadAnimGraph::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the id from the parameters
        uint32 animGraphID = MCORE_INVALIDINDEX32;
        if (parameters.CheckIfHasParameter("animGraphID"))
        {
            animGraphID = parameters.GetValueAsInt("animGraphID", MCORE_INVALIDINDEX32);
            if (EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID))
            {
                outResult = AZStd::string::format("Cannot import anim graph. Anim graph ID %i is already in use.", animGraphID);
                return false;
            }
        }

        // Get the filename of the anim graph asset.
        AZStd::string filename = parameters.GetValue("filename", this);
        if (m_relocateFilenameFunction)
        {
            m_relocateFilenameFunction(filename);
        }
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);
        // Resolve the filename if it starts with a path alias
        if (filename.starts_with('@'))
        {
            filename = EMotionFX::EMotionFXManager::ResolvePath(filename.c_str());
        }

        // Check if the anim graph got already loaded via the command system.
        const size_t numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (size_t i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);
            if (animGraph->GetFileNameString() == filename &&
                !animGraph->GetIsOwnedByRuntime() &&
                !animGraph->GetIsOwnedByAsset())
            {
                // In case the anim graph is already loaded, place its id into the result string for following command candidates
                // that might use %LASTRESULT%. As the command is succeeding we have to make sure the output string is valid as well.
                AZStd::to_string(outResult, animGraph->GetID());
                return true;
            }
        }

        // load anim graph from file
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetImporter().LoadAnimGraph(filename.c_str());
        if (!animGraph)
        {
            outResult = AZStd::string::format("Failed to load anim graph from %s.", filename.c_str());
            return false;
        }

        // set the id in case we have specified it as parameter
        if (animGraphID != MCORE_INVALIDINDEX32)
        {
            animGraph->SetID(animGraphID);
        }

        // in case we are in a redo call assign the previously used id
        if (m_oldAnimGraphId != MCORE_INVALIDINDEX32)
        {
            animGraph->SetID(m_oldAnimGraphId);
        }
        m_oldAnimGraphId = animGraph->GetID();

        animGraph->RecursiveInvalidateUniqueDatas();

        // return the id of the newly created anim graph
        AZStd::to_string(outResult, animGraph->GetID());

        AZStd::string removedConnections;
        animGraph->FindAndRemoveCycles(&removedConnections);
        if (!removedConnections.empty())
        {
            GetCommandManager()->AddError("The following connections: " + removedConnections + "where removed because they were producing cycles.");
        }

        // mark the workspace as dirty
        m_oldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        // automatically select the anim graph after loading it
        AZStd::string resultString;
        GetCommandManager()->ExecuteCommandInsideCommand("Unselect -animGraphIndex SELECT_ALL", resultString);
        GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("Select -animGraphID %d", animGraph->GetID()), resultString);

        return true;
    }


    // undo the command
    bool CommandLoadAnimGraph::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        // get the anim graph the command created
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(m_oldAnimGraphId);
        if (animGraph == nullptr)
        {
            outResult = AZStd::string::format("Cannot undo load anim graph command. Previously used anim graph id '%i' is not valid.", m_oldAnimGraphId);
            return false;
        }

        // Remove the newly created anim graph.
        const bool result = GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("RemoveAnimGraph -animGraphID %i", m_oldAnimGraphId), outResult);

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(m_oldWorkspaceDirtyFlag);

        return result;
    }


    // init the syntax of the command
    void CommandLoadAnimGraph::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("filename", "The filename of the anim graph file.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("animGraphID", "The id to assign to the newly loaded anim graph.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
    }


    // get the description
    const char* CommandLoadAnimGraph::GetDescription() const
    {
        return "This command loads a anim graph to the given file.";
    }

    //-------------------------------------------------------------------------------------
    // Create a new anim graph
    //-------------------------------------------------------------------------------------

    // constructor
    CommandCreateAnimGraph::CommandCreateAnimGraph(MCore::Command* orgCommand)
        : MCore::Command("CreateAnimGraph", orgCommand)
    {
        m_previouslyUsedId = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandCreateAnimGraph::~CommandCreateAnimGraph()
    {
    }


    // execute
    bool CommandCreateAnimGraph::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZStd::string resultString;

        // create the anim graph
        EMotionFX::AnimGraph* animGraph = aznew EMotionFX::AnimGraph();

        // create the root state machine object
        EMotionFX::AnimGraphObject* rootSMObject = EMotionFX::AnimGraphObjectFactory::Create(azrtti_typeid<EMotionFX::AnimGraphStateMachine>(), animGraph);
        if (!rootSMObject)
        {
            MCore::LogWarning("Cannot instantiate root state machine for new anim graph.");
            return false;
        }

        // type-cast the object to a state machine and set it as root state machine
        EMotionFX::AnimGraphStateMachine* rootSM = static_cast<EMotionFX::AnimGraphStateMachine*>(rootSMObject);
        animGraph->SetRootStateMachine(rootSM);

        animGraph->SetDirtyFlag(true);

        // in case we are in a redo call assign the previously used id
        if (parameters.CheckIfHasParameter("animGraphID"))
        {
            animGraph->SetID(parameters.GetValueAsInt("animGraphID", this));
        }
        if (m_previouslyUsedId != MCORE_INVALIDINDEX32)
        {
            animGraph->SetID(m_previouslyUsedId);
        }
        m_previouslyUsedId = animGraph->GetID();

        animGraph->RecursiveReinit();
        animGraph->RecursiveInvalidateUniqueDatas();

        // register leader animgraph
        GetCommandManager()->ExecuteCommandInsideCommand("Unselect -animGraphIndex SELECT_ALL", resultString);
        GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("Select -animGraphID %d", animGraph->GetID()), resultString);

        // return the id of the newly created anim graph
        AZStd::to_string(outResult, animGraph->GetID());

        // mark the workspace as dirty
        m_oldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        return true;
    }


    // undo the command
    bool CommandCreateAnimGraph::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        // get the anim graph the command created
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(m_previouslyUsedId);
        if (animGraph == nullptr)
        {
            outResult = AZStd::string::format("Cannot undo create anim graph command. Previously used anim graph id '%i' is not valid.", m_previouslyUsedId);
            return false;
        }

        // remove the newly created anim graph again
        const bool result = GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("RemoveAnimGraph -animGraphID %i", m_previouslyUsedId), outResult);

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(m_oldWorkspaceDirtyFlag);

        return result;
    }


    // init the syntax of the command
    void CommandCreateAnimGraph::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddParameter("animGraphID",    "The id of the anim graph to remove.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
    }


    // get the description
    const char* CommandCreateAnimGraph::GetDescription() const
    {
        return "This command creates a new anim graph.";
    }


    //-------------------------------------------------------------------------------------
    // Remove the given anim graph
    //-------------------------------------------------------------------------------------

    // constructor
    CommandRemoveAnimGraph::CommandRemoveAnimGraph(MCore::Command* orgCommand)
        : MCore::Command("RemoveAnimGraph", orgCommand)
    {
    }


    // destructor
    CommandRemoveAnimGraph::~CommandRemoveAnimGraph()
    {
    }


    // execute
    bool CommandRemoveAnimGraph::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the anim graph id from the string and check if it is valid
        const AZStd::string animGraphIDString = parameters.GetValue("animGraphID", this);
        if (animGraphIDString == "SELECT_ALL")
        {
            bool someAnimGraphRemoved = false;

            // remove all anim graphs, to do so we will iterate over them and issue an internal command for
            // that specific ID. This way we don't need to add complexity to this command to deal with all 
            // the anim graph's undo data
            for (size_t i = 0; i < EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();)
            {
                EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);
                if (!animGraph->GetIsOwnedByRuntime() && !animGraph->GetIsOwnedByAsset())
                {
                    m_oldFileNamesAndIds.emplace_back(animGraph->GetFileName(), animGraph->GetID());
                    if (!GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("RemoveAnimGraph -animGraphID %d", animGraph->GetID()), outResult))
                    {
                        m_oldFileNamesAndIds.pop_back();
                        return false;
                    }
                    someAnimGraphRemoved = true;
                    i = 0; // we start again to handle the case where an anim graph was removed because it was in a reference node
                }
                else
                {
                    ++i;
                }
            }

            if (someAnimGraphRemoved)
            {
                m_oldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
                GetCommandManager()->SetWorkspaceDirtyFlag(true);
            }
            
            return true;
        }

        const uint32 animGraphID = AzFramework::StringFunc::ToInt(animGraphIDString.c_str());
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (animGraph == nullptr)
        {
            outResult = AZStd::string::format("Cannot remove anim graph. Anim graph id '%i' is not valid.", animGraphID);
            return false;
        }

        // unselect all anim graphs
        GetCommandManager()->ExecuteCommandInsideCommand("Unselect -animGraphIndex SELECT_ALL", outResult);

        // remove the given anim graph
        m_oldFileNamesAndIds.emplace_back(animGraph->GetFileName(), animGraph->GetID());
        size_t oldIndex = EMotionFX::GetAnimGraphManager().FindAnimGraphIndex(animGraph);

        // iterate through all anim graph instances and remove the ones that depend on the anim graph to be removed
        for (size_t i = 0; i < EMotionFX::GetAnimGraphManager().GetNumAnimGraphInstances(); )
        {
            // in case the assigned anim graph instance belongs to the anim graph to remove, remove it and unassign it from all actor instances
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetAnimGraphManager().GetAnimGraphInstance(i);
            if (animGraphInstance->GetAnimGraph() == animGraph)
            {
                EMotionFX::GetAnimGraphManager().RemoveAnimGraphInstance(animGraphInstance);
            }
            else
            {
                i++;
            }
        }

        // get rid of the anim graph
        EMotionFX::GetAnimGraphManager().RemoveAnimGraph(animGraph);

        // Reselect the anim graph at the index of the removed one if possible.
        const size_t numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (size_t indexToSelect = oldIndex; indexToSelect < numAnimGraphs; indexToSelect--)
        {
            EMotionFX::AnimGraph* selectionCandidate = EMotionFX::GetAnimGraphManager().GetAnimGraph(indexToSelect);
            if (!selectionCandidate->GetIsOwnedByRuntime())
            {
                const EMotionFX::AnimGraph* newSelectedAnimGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(indexToSelect);
                GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("Select -animGraphID %d", newSelectedAnimGraph->GetID()), outResult);
                break;
            }
        }

        // mark the workspace as dirty
        m_oldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        return true;
    }


    // undo the command
    bool CommandRemoveAnimGraph::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        bool result = true;
        for (const AZStd::pair<AZStd::string, uint32>& oldFileNameAndId : m_oldFileNamesAndIds)
        {
            if (!oldFileNameAndId.first.empty())
            {
                result |= GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("LoadAnimGraph -filename \"%s\" -animGraphID %i", oldFileNameAndId.first.c_str(), oldFileNameAndId.second), outResult);
            }
            else
            {
                result |= GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("CreateAnimGraph -animGraphID %i", oldFileNameAndId.second), outResult);
            }
        }

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(m_oldWorkspaceDirtyFlag);

        return result;
    }


    // init the syntax of the command
    void CommandRemoveAnimGraph::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the anim graph to remove.", MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    // get the description
    const char* CommandRemoveAnimGraph::GetDescription() const
    {
        return "This command removes the given anim graph.";
    }

    //-------------------------------------------------------------------------------------
    // Activate the given anim graph
    //-------------------------------------------------------------------------------------
    CommandActivateAnimGraph::CommandActivateAnimGraph(MCore::Command* orgCommand)
        : MCore::Command(s_activateAnimGraphCmdName, orgCommand)
    {
        m_actorInstanceId  = MCORE_INVALIDINDEX32;
        m_oldAnimGraphUsed = MCORE_INVALIDINDEX32;
        m_oldMotionSetUsed = MCORE_INVALIDINDEX32;
    }

    CommandActivateAnimGraph::~CommandActivateAnimGraph()
    {
    }

    bool CommandActivateAnimGraph::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Get the actor instance by id.
        EMotionFX::ActorInstance* actorInstance = nullptr;
        if (parameters.CheckIfHasParameter("actorInstanceID"))
        {
            const uint32 actorInstanceID = parameters.GetValueAsInt("actorInstanceID", this);
            actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
            if (!actorInstance)
            {
                outResult = AZStd::string::format("Cannot activate anim graph. Actor instance id '%i' is not valid.", actorInstanceID);
                return false;
            }
        }
        else
        {
            outResult = "Cannot activate anim graph. Actor instance parameter must be specified.";
            return false;
        }

        // Get the anim graph to activate.
        EMotionFX::AnimGraph* animGraph = nullptr;
        if (parameters.CheckIfHasParameter("animGraphID"))
        {
            const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
            if (animGraphID != MCORE_INVALIDINDEX32)
            {
                animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
                if (!animGraph)
                {
                    outResult = AZStd::string::format("Cannot activate anim graph. Anim graph id '%i' is not valid.", animGraphID);
                    return false;
                }
            }
        }
        else
        {
            outResult = "Cannot activate anim graph. Anim graph parameter must be specified.";
            return false;
        }

        // Get the motion set to use.
        EMotionFX::MotionSet* motionSet = nullptr;
        if (parameters.CheckIfHasParameter("motionSetID"))
        {
            const uint32 motionSetID = parameters.GetValueAsInt("motionSetID", this);
            motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
            if (!motionSet)
            {
                outResult = AZStd::string::format("Cannot activate anim graph. Motion set id '%i' is not valid.", motionSetID);
                return false;
            }
        }
        else
        {
            outResult = "Cannot activate anim graph. Motion set parameter must be specified.";
            return false;
        }

        // store the actor instance ID
        m_actorInstanceId = actorInstance->GetID();

        // get the motion system from the actor instance
        EMotionFX::MotionSystem* motionSystem = actorInstance->GetMotionSystem();

        // remove all motion instances from this motion system
        const size_t numMotionInstances = motionSystem->GetNumMotionInstances();
        for (size_t j = 0; j < numMotionInstances; ++j)
        {
            EMotionFX::MotionInstance* motionInstance = motionSystem->GetMotionInstance(j);
            motionSystem->RemoveMotionInstance(motionInstance);
        }

        // get the visualize scale from the string
        const float visualizeScale = parameters.GetValueAsFloat("visualizeScale", this);

        EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
        if (animGraphInstance)
        {
            // get the anim graph instance anim graph and motion set
            EMotionFX::AnimGraph* animGraphInstanceAnimGraph = animGraphInstance->GetAnimGraph();
            EMotionFX::MotionSet* animGraphInstanceMotionSet = animGraphInstance->GetMotionSet();

            // store the currently used anim graph ID, motion set ID and the visualize scale
            m_oldAnimGraphUsed = animGraphInstanceAnimGraph->GetID();
            m_oldMotionSetUsed = (animGraphInstanceMotionSet) ? animGraphInstanceMotionSet->GetID() : MCORE_INVALIDINDEX32;
            m_oldVisualizeScaleUsed = animGraphInstance->GetVisualizeScale();

            // check if the anim graph is valid
            if (animGraph)
            {
                // check if the anim graph is not the same or if the motion set is not the same
                // This following line is currently commented, as when we Stop an anim graph there is no other way to start it nicely.
                // So we just recreate it for now.
                // At a later stage we can make proper starting support to prevent recreation.
                //if (animGraphInstanceAnimGraph != animGraph || animGraphInstanceMotionSet != motionSet)
                {
                    // destroy the current anim graph instance
                    animGraphInstance->Destroy();

                    // create a new anim graph instance
                    animGraphInstance = EMotionFX::AnimGraphInstance::Create(animGraph, actorInstance, motionSet);
                    animGraphInstance->SetVisualizeScale(visualizeScale);

                    actorInstance->SetAnimGraphInstance(animGraphInstance);
                    animGraphInstance->RecursiveInvalidateUniqueDatas();
                }
            }
            else
            {
                animGraphInstance->Destroy();
                actorInstance->SetAnimGraphInstance(nullptr);
            }
        }
        else // no one anim graph instance set on the actor instance, create a new one
        {
            // store the currently used ID as invalid
            m_oldAnimGraphUsed = MCORE_INVALIDINDEX32;
            m_oldMotionSetUsed = MCORE_INVALIDINDEX32;

            // check if the anim graph is valid
            if (animGraph)
            {
                // create a new anim graph instance
                animGraphInstance = EMotionFX::AnimGraphInstance::Create(animGraph, actorInstance, motionSet);
                animGraphInstance->SetVisualizeScale(visualizeScale);

                actorInstance->SetAnimGraphInstance(animGraphInstance);
                animGraphInstance->RecursiveInvalidateUniqueDatas();
            }
        }

        // return the id of the newly created anim graph
        AZStd::to_string(outResult, animGraph ? animGraph->GetID() : MCORE_INVALIDINDEX32);

        // mark the workspace as dirty
        m_oldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        AZStd::string resultString;
        GetCommandManager()->ExecuteCommandInsideCommand("Unselect -animGraphIndex SELECT_ALL", resultString);
        if (animGraph)
        {
            GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("Select -animGraphID %d", animGraph->GetID()), resultString);
        }

        if (parameters.GetValueAsBool("startRecording", this))
        {
            EMotionFX::Recorder::RecordSettings settings;
            settings.m_fps = 1000000;
            settings.m_recordTransforms = true;
            settings.m_recordAnimGraphStates = true;
            settings.m_recordNodeHistory = true;
            settings.m_recordScale = true;
            settings.m_initialAnimGraphAnimBytes = 4 * 1024 * 1024; // 4 mb
            EMotionFX::GetRecorder().StartRecording(settings);
        }

        // done
        return true;
    }

    bool CommandActivateAnimGraph::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        // get the actor instance id and check if it is valid
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(m_actorInstanceId);
        if (actorInstance == nullptr)
        {
            outResult = AZStd::string::format("Cannot undo activate anim graph. Actor instance id '%i' is not valid.", m_actorInstanceId);
            return false;
        }

        // get the anim graph, invalid index is a special case to allow the anim graph to be nullptr
        EMotionFX::AnimGraph* animGraph;
        if (m_oldAnimGraphUsed == MCORE_INVALIDINDEX32)
        {
            animGraph = nullptr;
        }
        else
        {
            animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(m_oldAnimGraphUsed);
            if (animGraph == nullptr)
            {
                outResult = AZStd::string::format("Cannot undo activate anim graph. Anim graph id '%i' is not valid.", m_oldAnimGraphUsed);
                return false;
            }
        }

        // get the motion set, invalid index is a special case to allow the motion set to be nullptr
        EMotionFX::MotionSet* motionSet;
        if (m_oldMotionSetUsed == MCORE_INVALIDINDEX32)
        {
            motionSet = nullptr;
        }
        else
        {
            motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(m_oldMotionSetUsed);
            if (motionSet == nullptr)
            {
                outResult = AZStd::string::format("Cannot undo activate anim graph. Motion set id '%i' is not valid.", m_oldMotionSetUsed);
                return false;
            }
        }

        // get the motion system from the actor instance
        EMotionFX::MotionSystem* motionSystem = actorInstance->GetMotionSystem();

        // remove all motion instances from this motion system
        const size_t numMotionInstances = motionSystem->GetNumMotionInstances();
        for (size_t j = 0; j < numMotionInstances; ++j)
        {
            EMotionFX::MotionInstance* motionInstance = motionSystem->GetMotionInstance(j);
            motionSystem->RemoveMotionInstance(motionInstance);
        }

        // get the current anim graph instance
        EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();

        // check if one anim graph instance is set on the actor instance
        if (animGraphInstance)
        {
            // check if the anim graph is valid
            if (animGraph)
            {
                // check if the anim graph is not the same or if the motion set is not the same
                if (animGraphInstance->GetAnimGraph() != animGraph || animGraphInstance->GetMotionSet() != motionSet)
                {
                    // destroy the current anim graph instance
                    animGraphInstance->Destroy();

                    // create a new anim graph instance
                    animGraphInstance = EMotionFX::AnimGraphInstance::Create(animGraph, actorInstance, motionSet);
                    animGraphInstance->SetVisualizeScale(m_oldVisualizeScaleUsed);

                    actorInstance->SetAnimGraphInstance(animGraphInstance);
                    animGraphInstance->RecursiveInvalidateUniqueDatas();
                }
            }
            else
            {
                animGraphInstance->Destroy();
                actorInstance->SetAnimGraphInstance(nullptr);
            }
        }
        else // no one anim graph instance set on the actor instance, create a new one
        {
            // check if the anim graph is valid
            if (animGraph)
            {
                // create a new anim graph instance
                animGraphInstance = EMotionFX::AnimGraphInstance::Create(animGraph, actorInstance, motionSet);
                animGraphInstance->SetVisualizeScale(m_oldVisualizeScaleUsed);

                actorInstance->SetAnimGraphInstance(animGraphInstance);
                animGraphInstance->RecursiveInvalidateUniqueDatas();
            }
        }

        // return the id of the newly created anim graph
        AZStd::to_string(outResult, animGraph ? animGraph->GetID() : MCORE_INVALIDINDEX32);

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(m_oldWorkspaceDirtyFlag);

        MCore::CommandGroup commandGroup;
        commandGroup.AddCommandString("RecorderClear");
        commandGroup.AddCommandString("Unselect -animGraphIndex SELECT_ALL");
        if (animGraph)
        {
            commandGroup.AddCommandString(AZStd::string::format("Select -animGraphID %d", animGraph->GetID()));
        }
        AZStd::string resultString;
        GetCommandManager()->ExecuteCommandGroupInsideCommand(commandGroup, resultString);

        return true;
    }

    void CommandActivateAnimGraph::InitSyntax()
    {
        GetSyntax().ReserveParameters(5);
        GetSyntax().AddParameter("actorInstanceID", "The id of the actor instance.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("animGraphID", "The id of the anim graph.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("motionSetID", "The id of the motion set.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("visualizeScale", "The visualize scale.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "1.0");
        GetSyntax().AddParameter("startRecording", "Start a recording as soon as the activation occurs.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
    }

    const char* CommandActivateAnimGraph::GetDescription() const
    {
        return "This command activate the given anim graph.";
    }

    //-------------------------------------------------------------------------------------
    // Helper Functions
    //-------------------------------------------------------------------------------------

    void ClearAnimGraphsCommand(MCore::CommandGroup* commandGroup)
    {
        if (EMotionFX::GetAnimGraphManager().GetNumAnimGraphs() > 0)
        {
            const AZStd::string command = "RemoveAnimGraph -animGraphID SELECT_ALL";

            if (!commandGroup)
            {
                AZStd::string result;
                if (!GetCommandManager()->ExecuteCommand(command, result))
                {
                    AZ_Error("EMotionFX", false, result.c_str());
                }
            }
            else
            {
                commandGroup->AddCommandString(command);
            }
        }
    }


    void LoadAnimGraphsCommand(const AZStd::vector<AZStd::string>& filenames, bool reload)
    {
        if (filenames.empty())
        {
            return;
        }

        const size_t numFileNames = filenames.size();

        AZStd::string commandString = AZStd::string::format("%s %zu anim graph%s", reload ? "Reload" : "Load", numFileNames, numFileNames > 1 ? "s" : "");
        MCore::CommandGroup commandGroup(commandString.c_str());

        // Iterate over all filenames and load the anim graphs.
        for (size_t i = 0; i < numFileNames; ++i)
        {
            // In case we want to reload the same anim graph remove the old version first.
            if (reload)
            {
                // Remove all anim graphs with the given filename.
                const size_t numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
                for (size_t j = 0; j < numAnimGraphs; ++j)
                {
                    const EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(j);

                    if (animGraph &&
                        !animGraph->GetIsOwnedByRuntime() && !animGraph->GetIsOwnedByAsset() &&
                        AzFramework::StringFunc::Equal(animGraph->GetFileName(), filenames[i].c_str(), false /* no case */))
                    {
                        commandString = AZStd::string::format("RemoveAnimGraph -animGraphID %d", animGraph->GetID());
                        commandGroup.AddCommandString(commandString);
                    }
                }
            }

            commandString = AZStd::string::format("LoadAnimGraph -filename \"%s\"", filenames[i].c_str());
            commandGroup.AddCommandString(commandString);
        }

        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        GetCommandManager()->ClearHistory();
    }
} // namesapce EMotionFX
