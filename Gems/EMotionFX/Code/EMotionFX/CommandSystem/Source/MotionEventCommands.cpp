/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "MotionEventCommands.h"

#include <AzCore/Serialization/Locale.h>

#include "CommandManager.h"
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/MotionManager.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/TwoStringEventData.h>

namespace CommandSystem
{
    AZ_CLASS_ALLOCATOR_IMPL(CommandCreateMotionEventTrack, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(CommandClearMotionEvents, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(CommandCreateMotionEvent, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(CommandAdjustMotionEventTrack, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(CommandAdjustMotionEvent, EMotionFX::CommandAllocator)

    // add a new motion event
    void CommandHelperAddMotionEvent(const EMotionFX::Motion* motion, const char* trackName, float startTime, float endTime, const EMotionFX::EventDataSet& eventDatas, MCore::CommandGroup* commandGroup)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // Ensures that %f uses "." as decimal separator
        // make sure the motion is valid
        if (motion == nullptr)
        {
            return;
        }

        // create our command group
        MCore::CommandGroup internalCommandGroup("Add motion event");

        // execute the create motion event command
        AZStd::string command;
        command = AZStd::string::format(
            "CreateMotionEvent "
            "-motionID %i "
            "-eventTrackName \"%s\" "
            "-startTime %f "
            "-endTime %f ",
            motion->GetID(),
            trackName,
            startTime,
            endTime
        );

        if (!eventDatas.empty())
        {
            const AZ::Outcome<AZStd::string> serializedEventData = MCore::ReflectionSerializer::Serialize(&eventDatas);
            if (serializedEventData.IsSuccess())
            {
                command += "-eventDatas \"" + serializedEventData.GetValue() + '"';
            }
        }


        // add the command to the command group
        if (commandGroup == nullptr)
        {
            internalCommandGroup.AddCommandString(command.c_str());
        }
        else
        {
            commandGroup->AddCommandString(command.c_str());
        }

        // execute the group command
        if (commandGroup == nullptr)
        {
            AZStd::string outResult;
            if (GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, outResult) == false)
            {
                MCore::LogError(outResult.c_str());
            }
        }
    }

    //--------------------------------------------------------------------------------
    // CommandCreateMotionEventTrack
    //--------------------------------------------------------------------------------

    CommandClearMotionEvents::CommandClearMotionEvents(MCore::Command* originalCommand)
        : MCore::Command("ClearMotionEvents", originalCommand)
    {
    }

    // execute
    bool CommandClearMotionEvents::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        // check if the motion is valid and return failure in case it is not
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(m_motionID);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot create motion event track. Motion with id='%i' does not exist.", m_motionID);
            return false;
        }

        // get the motion event table as well as the event track name
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();

        // remove all motion event tracks including all motion events
        eventTable->RemoveAllTracks();

        // set the dirty flag
        AZStd::string commandString;
        commandString = AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", m_motionID);
        return GetCommandManager()->ExecuteCommandInsideCommand(commandString.c_str(), outResult);
    }


    // undo the command
    bool CommandClearMotionEvents::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    // init the syntax of the command
    void CommandClearMotionEvents::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("motionID", "The id of the motion.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    bool CommandClearMotionEvents::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        m_motionID = parameters.GetValueAsInt("motionID", this);
        return true;
    }

    // get the description
    const char* CommandClearMotionEvents::GetDescription() const
    {
        return "Removes all the motion event tracks including all motion events for the given motion.";
    }


    void CommandClearMotionEvents::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandClearMotionEvents, MCore::Command, MotionIdCommandMixin>()
            ->Version(1)
            ;
    }

    //--------------------------------------------------------------------------------
    // CommandCreateMotionEventTrack
    //--------------------------------------------------------------------------------

    // constructor
    CommandCreateMotionEventTrack::CommandCreateMotionEventTrack(MCore::Command* orgCommand)
        : MCore::Command("CreateMotionEventTrack", orgCommand)
    {
    }

    void CommandCreateMotionEventTrack::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandCreateMotionEventTrack, MCore::Command, MotionIdCommandMixin>()
            ->Version(1)
            ->Field("eventTrackName", &CommandCreateMotionEventTrack::m_eventTrackName)
            ->Field("eventTrackIndex", &CommandCreateMotionEventTrack::m_eventTrackIndex)
            ->Field("isEnabled", &CommandCreateMotionEventTrack::m_isEnabled)
            ;
    }

    // execute
    bool CommandCreateMotionEventTrack::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        // find the motion
        EMotionFX::Motion*  motion = EMotionFX::GetMotionManager().FindMotionByID(m_motionID);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot create motion event track. Motion with id='%i' does not exist.", m_motionID);
            return false;
        }

        // get the motion event table as well as the event track name
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();

        // check if the track is already there, if not create it
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(m_eventTrackName.c_str());
        if (eventTrack == nullptr)
        {
            eventTrack = EMotionFX::MotionEventTrack::Create(m_eventTrackName.c_str(), motion);
        }

        // add the motion event track
        if (m_eventTrackIndex)
        {
            eventTable->InsertTrack(m_eventTrackIndex.value(), eventTrack);
        }
        else
        {
            eventTable->AddTrack(eventTrack);
        }

        // set the enable flag
        if (m_isEnabled)
        {
            eventTrack->SetIsEnabled(m_isEnabled.value());
        }

        // make sure there is a sync track
        motion->GetEventTable()->AutoCreateSyncTrack(motion);

        // set the dirty flag
        const AZStd::string valueString = AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", m_motionID);
        return GetCommandManager()->ExecuteCommandInsideCommand(valueString.c_str(), outResult);
    }


    // undo the command
    bool CommandCreateMotionEventTrack::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        AZStd::string eventTrackName;
        parameters.GetValue("eventTrackName", this, &eventTrackName);

        AZStd::string command;
        command = AZStd::string::format("RemoveMotionEventTrack -motionID %i -eventTrackName \"%s\"", m_motionID, eventTrackName.c_str());
        return GetCommandManager()->ExecuteCommandInsideCommand(command.c_str(), outResult);
    }


    // init the syntax of the command
    void CommandCreateMotionEventTrack::InitSyntax()
    {
        GetSyntax().ReserveParameters(4);
        GetSyntax().AddRequiredParameter("motionID",         "The id of the motion.",                                        MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("eventTrackName",   "The name of the motion event track.",                          MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("index",            "The index of the event track in the event table.",             MCore::CommandSyntax::PARAMTYPE_INT,        "-1");
        GetSyntax().AddParameter("enabled",          "Flag which indicates if the event track is enabled or not.",   MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "true");
    }

    bool CommandCreateMotionEventTrack::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        MotionIdCommandMixin::SetCommandParameters(parameters);

        parameters.GetValue("eventTrackName", this, &m_eventTrackName);

        if (parameters.CheckIfHasParameter("index"))
        {
            m_eventTrackIndex = parameters.GetValueAsInt("index", this);
        }

        if (parameters.CheckIfHasParameter("enabled"))
        {
            m_isEnabled = parameters.GetValueAsBool("enabled", this);
        }

        return true;
    }

    // get the description
    const char* CommandCreateMotionEventTrack::GetDescription() const
    {
        return "Create a motion event track with the given name for the given motion.";
    }

    //--------------------------------------------------------------------------------
    // CommandRemoveMotionEventTrack
    //--------------------------------------------------------------------------------

    // constructor
    CommandRemoveMotionEventTrack::CommandRemoveMotionEventTrack(MCore::Command* orgCommand)
        : MCore::Command("RemoveMotionEventTrack", orgCommand)
    {
        m_oldTrackIndex = InvalidIndex;
    }


    // destructor
    CommandRemoveMotionEventTrack::~CommandRemoveMotionEventTrack()
    {
    }


    // execute
    bool CommandRemoveMotionEventTrack::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the motion id and find the corresponding object
        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        // check if the motion is valid and return failure in case it is not
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot remove motion event track. Motion with id='%i' does not exist.", motionID);
            return false;
        }

        // get the motion event table as well as the event track name
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();

        AZStd::string valueString;
        parameters.GetValue("eventTrackName", this, &valueString);

        // check if the motion event track is valid and return failure in case it is not
        const AZ::Outcome<size_t> eventTrackIndex = eventTable->FindTrackIndexByName(valueString.c_str());
        if (!eventTrackIndex)
        {
            outResult = AZStd::string::format("Cannot remove motion event track. Motion event track '%s' does not exist for motion with id='%i'.", valueString.c_str(), motionID);
            return false;
        }

        // store information for undo
        m_oldTrackIndex  = eventTrackIndex.GetValue();
        m_oldEnabled     = eventTable->GetTrack(eventTrackIndex.GetValue())->GetIsEnabled();

        // remove the motion event track
        eventTable->RemoveTrack(eventTrackIndex.GetValue());

        // set the dirty flag
        valueString = AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", motionID);
        return GetCommandManager()->ExecuteCommandInsideCommand(valueString.c_str(), outResult);
    }


    // undo the command
    bool CommandRemoveMotionEventTrack::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZStd::string eventTrackName;
        parameters.GetValue("eventTrackName", this, &eventTrackName);

        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        AZStd::string command;
        command = AZStd::string::format("CreateMotionEventTrack -motionID %i -eventTrackName \"%s\" -index %zu -enabled %s", motionID, eventTrackName.c_str(), m_oldTrackIndex, m_oldEnabled ? "true" : "false");
        return GetCommandManager()->ExecuteCommandInsideCommand(command.c_str(), outResult);
    }


    // init the syntax of the command
    void CommandRemoveMotionEventTrack::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("motionID",        "The id of the motion.",                MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("eventTrackName",  "The name of the motion event track.",  MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    // get the description
    const char* CommandRemoveMotionEventTrack::GetDescription() const
    {
        return "Remove a motion event track from the given motion.";
    }


    //--------------------------------------------------------------------------------
    // CommandAdjustMotionEventTrack
    //--------------------------------------------------------------------------------

    // constructor
    CommandAdjustMotionEventTrack::CommandAdjustMotionEventTrack(MCore::Command* orgCommand)
        : MCore::Command("AdjustMotionEventTrack", orgCommand)
    {
    }


    void CommandAdjustMotionEventTrack::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if(!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandAdjustMotionEventTrack, MCore::Command, MotionIdCommandMixin>()
            ->Field("eventTrackName", &CommandAdjustMotionEventTrack::m_eventTrackName)
            ->Field("newName", &CommandAdjustMotionEventTrack::m_newName)
            ->Field("enabled", &CommandAdjustMotionEventTrack::m_isEnabled)
            ;
    }

    // execute
    bool CommandAdjustMotionEventTrack::Execute(const MCore::CommandLine& /*parameters*/, AZStd::string& outResult)
    {
        // check if the motion is valid and return failure in case it is not
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(m_motionID);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust motion event track. Motion with id='%i' does not exist.", m_motionID);
            return false;
        }

        // get the motion event table
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();

        // check if the motion event track is valid and return failure in case it is not
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(m_eventTrackName.c_str());
        if (eventTrack == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust motion event track. Motion event track '%s' does not exist for motion with id='%i'.", m_eventTrackName.c_str(), m_motionID);
            return false;
        }

        if (m_newName)
        {
            m_oldName = eventTrack->GetName();
            eventTrack->SetName(m_newName.value().c_str());
        }

        if (m_isEnabled)
        {
            m_oldEnabled = eventTrack->GetIsEnabled();
            eventTrack->SetIsEnabled(m_isEnabled.value());
        }

        // set the dirty flag
        return GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", m_motionID).c_str(), outResult);
    }


    // undo the command
    bool CommandAdjustMotionEventTrack::Undo(const MCore::CommandLine& /*parameters*/, AZStd::string& outResult)
    {
        // check if the motion is valid and return failure in case it is not
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(m_motionID);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust motion event track. Motion with id='%i' does not exist.", m_motionID);
            return false;
        }

        // get the motion event table
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();

        // get the event track which we're working on
        EMotionFX::MotionEventTrack* eventTrack = nullptr;
        if (m_newName)
        {
            eventTrack = eventTable->FindTrackByName(m_newName.value().c_str());
        }
        else
        {
            eventTrack = eventTable->FindTrackByName(m_eventTrackName.c_str());
        }

        // check if the motion event track is valid and return failure in case it is not
        if (!eventTrack)
        {
            outResult = AZStd::string::format("Cannot undo adjust motion event track. Motion event track does not exist for motion with id='%i'.", m_motionID);
            return false;
        }

        if (m_newName.has_value())
        {
            eventTrack->SetName(m_oldName.c_str());
        }

        if (m_isEnabled.has_value())
        {
            eventTrack->SetIsEnabled(m_oldEnabled);
        }

        // undo remove of the motion
        return GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", m_motionID).c_str(), outResult);
    }


    // init the syntax of the command
    void CommandAdjustMotionEventTrack::InitSyntax()
    {
        GetSyntax().ReserveParameters(4);
        GetSyntax().AddRequiredParameter("motionID",         "The id of the motion.",                                            MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("eventTrackName",   "The name of the motion event track.",                              MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("newName",          "The new name of the motion event track.",                          MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("enabled",          "True in case the motion event track is enabled, false if not.",    MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
    }


    bool CommandAdjustMotionEventTrack::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        MotionIdCommandMixin::SetCommandParameters(parameters);

        parameters.GetValue("eventTrackName", this, &m_eventTrackName);

        if (parameters.CheckIfHasParameter("newName"))
        {
            m_newName = parameters.GetParameterValue(parameters.FindParameterIndex("newName"));
        }
        if (parameters.CheckIfHasParameter("enabled"))
        {
            m_isEnabled = parameters.GetValueAsBool("enabled", this);
        }

        return true;
    }

    // get the description
    const char* CommandAdjustMotionEventTrack::GetDescription() const
    {
        return "Adjust the attributes of a given motion event track.";
    }

    //--------------------------------------------------------------------------------
    // CommandCreateMotionEvent
    //--------------------------------------------------------------------------------

    // constructor
    CommandCreateMotionEvent::CommandCreateMotionEvent(MCore::Command* orgCommand)
        : MCore::Command("CreateMotionEvent", orgCommand)
    {
    }


    void CommandCreateMotionEvent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if(!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandCreateMotionEvent, MCore::Command, MotionIdCommandMixin>()
            ->Field("eventTrackName", &CommandCreateMotionEvent::m_eventTrackName)
            ->Field("startTime", &CommandCreateMotionEvent::m_startTime)
            ->Field("endTime", &CommandCreateMotionEvent::m_endTime)
            ->Field("eventDatas", &CommandCreateMotionEvent::m_eventDatas)
            ;
    }

    // execute
    bool CommandCreateMotionEvent::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        // check if the motion is valid and return failure in case it is not
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(m_motionID);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot create motion event. Motion with id='%i' does not exist.", m_motionID);
            return false;
        }

        // get the motion event table and find the track on which we will work on
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(m_eventTrackName.c_str());

        // check if the motion event track is valid and return failure in case it is not
        if (eventTrack == nullptr)
        {
            outResult = AZStd::string::format("Cannot create motion event. Motion event track '%s' does not exist for motion with id='%i'.", m_eventTrackName.c_str(), m_motionID);
            return false;
        }

        // Deserialize the event data
        if (m_serializedEventData)
        {
            EMotionFX::EventDataSet eventDataSet;
            if (!MCore::ReflectionSerializer::Deserialize(&eventDataSet, m_serializedEventData.value()))
            {
                outResult = AZStd::string::format("Cannot deserialize -eventData parameter");
                return false;
            }

            m_eventDatas = AZStd::move(eventDataSet);
        }

        // add the motion event and check if everything worked fine
        m_motionEventNr = eventTrack->AddEvent(m_startTime, m_endTime, m_eventDatas.value_or(EMotionFX::EventDataSet()));

        if (m_motionEventNr == InvalidIndex)
        {
            outResult = AZStd::string::format("Cannot create motion event. The returned motion event index is not valid.");
            return false;
        }

        // set the dirty flag
        return GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", m_motionID).c_str(), outResult);
    }


    // undo the command
    bool CommandCreateMotionEvent::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        const AZStd::string command = AZStd::string::format("RemoveMotionEvent -motionID %i -eventTrackName \"%s\" -eventNr %zu", m_motionID, m_eventTrackName.c_str(), m_motionEventNr);
        return GetCommandManager()->ExecuteCommandInsideCommand(command.c_str(), outResult);
    }


    // init the syntax of the command
    void CommandCreateMotionEvent::InitSyntax()
    {
        GetSyntax().ReserveParameters(6);
        GetSyntax().AddRequiredParameter("motionID", "The id of the motion.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("eventTrackName", "The name of the motion event track.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("startTime", "The start time value, in seconds, when the motion event should start.", MCore::CommandSyntax::PARAMTYPE_FLOAT);
        GetSyntax().AddRequiredParameter("endTime", "The end time value, in seconds, when the motion event should end. When this is equal to the start time value we won't trigger an end event, but only a start event at the specified time.", MCore::CommandSyntax::PARAMTYPE_FLOAT);
        GetSyntax().AddParameter("eventDatas", "A serialized string of a vector of EventData subclasses, containing the parameters that should be sent with the event.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    bool CommandCreateMotionEvent::SetCommandParameters(const MCore::CommandLine& commandLine)
    {
        MotionIdCommandMixin::SetCommandParameters(commandLine);

        commandLine.GetValue("eventTrackName", this, m_eventTrackName);
        m_startTime = commandLine.GetValueAsFloat("startTime", this);
        m_endTime = commandLine.GetValueAsFloat("endTime", this);

        if (commandLine.CheckIfHasParameter("eventDatas"))
        {
            AZStd::string eventData;
            commandLine.GetValue("eventDatas", this, eventData);
            m_serializedEventData = eventData;
        }

        return true;
    }


    // get the description
    const char* CommandCreateMotionEvent::GetDescription() const
    {
        return "Create a motion event with the given parameters for the given motion.";
    }

    //--------------------------------------------------------------------------------
    // CommandRemoveMotionEvent
    //--------------------------------------------------------------------------------

    // constructor
    CommandRemoveMotionEvent::CommandRemoveMotionEvent(MCore::Command* orgCommand)
        : MCore::Command("RemoveMotionEvent", orgCommand)
    {
    }


    // destructor
    CommandRemoveMotionEvent::~CommandRemoveMotionEvent()
    {
    }


    // execute
    bool CommandRemoveMotionEvent::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the motion id and find the corresponding object
        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        // check if the motion is valid and return failure in case it is not
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot remove motion event. Motion with id='%i' does not exist.", motionID);
            return false;
        }

        // get the motion event table and find the track on which we will work on
        AZStd::string valueString;
        parameters.GetValue("eventTrackName", this, &valueString);
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(valueString.c_str());

        // check if the motion event track is valid and return failure in case it is not
        if (eventTrack == nullptr)
        {
            outResult = AZStd::string::format("Cannot remove motion event. Motion event track '%s' does not exist for motion with id='%i'.", valueString.c_str(), motionID);
            return false;
        }

        // get the event index and check if it is in range
        const int32 eventNr = parameters.GetValueAsInt("eventNr", this);
        if (eventNr < 0 || eventNr >= (int32)eventTrack->GetNumEvents())
        {
            outResult = AZStd::string::format("Cannot remove motion event. Motion event number '%i' is out of range.", eventNr);
            return false;
        }

        // get the motion event and store the old values of the motion event for undo
        const EMotionFX::MotionEvent& motionEvent = eventTrack->GetEvent(eventNr);
        m_oldStartTime = motionEvent.GetStartTime();
        m_oldEndTime = motionEvent.GetEndTime();
        m_oldEventDatas = motionEvent.GetEventDatas();

        // remove the motion event
        eventTrack->RemoveEvent(eventNr);

        // set the dirty flag
        valueString = AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", motionID);
        return GetCommandManager()->ExecuteCommandInsideCommand(valueString.c_str(), outResult);
    }


    // undo the command
    bool CommandRemoveMotionEvent::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZStd::string eventTrackName;
        parameters.GetValue("eventTrackName", this, &eventTrackName);

        const int32 motionID = parameters.GetValueAsInt("motionID", this);
        const EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (!motion)
        {
            outResult = AZStd::string::format("Unable to find motion with id %d", motionID);
            return false;
        }

        MCore::CommandGroup commandGroup;
        CommandHelperAddMotionEvent(motion, eventTrackName.c_str(), m_oldStartTime, m_oldEndTime, m_oldEventDatas, &commandGroup);
        return GetCommandManager()->ExecuteCommandGroupInsideCommand(commandGroup, outResult);
    }


    // init the syntax of the command
    void CommandRemoveMotionEvent::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        GetSyntax().AddRequiredParameter("motionID",        "The id of the motion.",                    MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("eventTrackName",  "The name of the motion event track.",      MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("eventNr",         "The index of the motion event to remove.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    // get the description
    const char* CommandRemoveMotionEvent::GetDescription() const
    {
        return "Remove a motion event from the given motion.";
    }



    //--------------------------------------------------------------------------------
    // CommandAdjustMotionEvent
    //--------------------------------------------------------------------------------

    // constructor
    CommandAdjustMotionEvent::CommandAdjustMotionEvent(MCore::Command* orgCommand)
        : MCore::Command("AdjustMotionEvent", orgCommand)
    {
    }


    void CommandAdjustMotionEvent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if(!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandAdjustMotionEvent, MCore::Command, MotionIdCommandMixin>()
            ->Field("eventTrackName", &CommandAdjustMotionEvent::m_eventTrackName)
            ->Field("startTime", &CommandAdjustMotionEvent::m_startTime)
            ->Field("endTime", &CommandAdjustMotionEvent::m_endTime)
            ->Field("eventDataAction", &CommandAdjustMotionEvent::m_eventDataAction)
            ->Field("eventData", &CommandAdjustMotionEvent::m_eventData)
            ;
    }


    // execute
    bool CommandAdjustMotionEvent::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        AZ::Outcome<EMotionFX::MotionEvent*> motionEventOutcome = GetMotionEvent();
        if (!motionEventOutcome.IsSuccess())
        {
            outResult = "Cannot find motion event with the parameters provided.";
            return false;
        }

        EMotionFX::MotionEvent* motionEvent = motionEventOutcome.GetValue();

        m_oldStartTime   = motionEvent->GetStartTime();
        m_oldEndTime     = motionEvent->GetEndTime();

        // adjust the event start time
        if (m_startTime.has_value())
        {
            if (m_startTime > motionEvent->GetEndTime())
            {
                motionEvent->SetEndTime(m_startTime.value());
            }

            motionEvent->SetStartTime(m_startTime.value());
        }

        // adjust the event end time
        if (m_endTime.has_value())
        {
            if (m_endTime < motionEvent->GetStartTime())
            {
                motionEvent->SetStartTime(m_endTime.value());
            }

            motionEvent->SetEndTime(m_endTime.value());
        }

        switch (m_eventDataAction)
        {
            case EventDataAction::Replace:
                m_oldEventData = motionEvent->GetEventDatas()[m_eventDataNr];
                motionEvent->SetEventData(m_eventDataNr, EMotionFX::GetEventManager().FindEventData(m_eventData.value()));
                break;
            case EventDataAction::Add:
                motionEvent->AppendEventData(EMotionFX::GetEventManager().FindEventData(m_eventData.value()));
                break;
            case EventDataAction::Remove:
                m_oldEventData = motionEvent->GetEventDatas()[m_eventDataNr];
                motionEvent->RemoveEventData(m_eventDataNr);
                break;
            case EventDataAction::None:
            default:
                break;
        }

        // set the dirty flag
        const AZStd::string valueString = AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", m_motionID);
        return GetCommandManager()->ExecuteCommandInsideCommand(valueString.c_str(), outResult);
    }


    // undo the command
    bool CommandAdjustMotionEvent::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        AZ::Outcome<EMotionFX::MotionEvent*> motionEventOutcome = GetMotionEvent();
        if (!motionEventOutcome.IsSuccess())
        {
            outResult = "Cannot find motion event with the parameters provided.";
            return false;
        }

        EMotionFX::MotionEvent* motionEvent = motionEventOutcome.GetValue();

        // undo start time change
        if (m_startTime.has_value())
        {
            motionEvent->SetStartTime(m_oldStartTime);
            motionEvent->SetEndTime(m_oldEndTime);
        }

        // undo end time change
        if (m_endTime.has_value())
        {
            motionEvent->SetStartTime(m_oldStartTime);
            motionEvent->SetEndTime(m_oldEndTime);
        }

        switch (m_eventDataAction)
        {
            case EventDataAction::Replace:
                motionEvent->SetEventData(m_eventDataNr, AZStd::move(m_oldEventData));
                break;
            case EventDataAction::Add:
                motionEvent->RemoveEventData(motionEvent->GetEventDatas().size() - 1);
                break;
            case EventDataAction::Remove:
                motionEvent->InsertEventData(m_eventDataNr, AZStd::move(m_oldEventData));
                break;
            case EventDataAction::None:
            default:
                break;
        }

        // undo remove of the motion
        const AZStd::string valueString { AZStd::string::format("AdjustMotion -motionID %i -dirtyFlag true", m_motionID) };
        return GetCommandManager()->ExecuteCommandInsideCommand(valueString.c_str(), outResult);
    }


    // init the syntax of the command
    void CommandAdjustMotionEvent::InitSyntax()
    {
        GetSyntax().ReserveParameters(5);
        GetSyntax().AddRequiredParameter("motionID",         "The id of the motion.",                                                                                                                                                                        MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("eventTrackName",   "The name of the motion event track.",                                                                                                                                                          MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("eventNr",          "The index of the motion event to modify.",                                                                                                                                                     MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("startTime",        "The start time value, in seconds, when the motion event should start.",                                                                                                                        MCore::CommandSyntax::PARAMTYPE_FLOAT,  "0.0");
        GetSyntax().AddParameter("endTime",          "The end time value, in seconds, when the motion event should end. When this is equal to the start time value we won't trigger an end event, but only a start event at the specified time.",    MCore::CommandSyntax::PARAMTYPE_FLOAT,  "0.0");
    }


    bool CommandAdjustMotionEvent::SetCommandParameters(const MCore::CommandLine& commandLine)
    {
        MotionIdCommandMixin::SetCommandParameters(commandLine);

        commandLine.GetValue("eventTrackName", this, &m_eventTrackName);
        m_eventNr = commandLine.GetValueAsInt("eventNr", this);

        if (commandLine.CheckIfHasParameter("startTime"))
        {
            m_startTime = commandLine.GetValueAsFloat("startTime", this);
        }
        if (commandLine.CheckIfHasParameter("endTime"))
        {
            m_endTime = commandLine.GetValueAsFloat("endTime", this);
        }

        return true;
    }


    // get the description
    const char* CommandAdjustMotionEvent::GetDescription() const
    {
        return "Adjust the attributes of a given motion event.";
    }

    AZ::Outcome<EMotionFX::MotionEvent*> CommandAdjustMotionEvent::GetMotionEvent() const
    {
        if (m_motionEvent)
        {
            return AZ::Success(m_motionEvent);
        }
        else {
            // check if the motion is valid and return failure in case it is not
            const EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(m_motionID);
            if (!motion)
            {
                return AZ::Failure();
            }

            // get the motion event table and find the track on which we will work on
            const EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();
            EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(m_eventTrackName.c_str());

            // check if the motion event track is valid and return failure in case it is not
            if (!eventTrack)
            {
                return AZ::Failure();
            }

            // get the event index and check if it is in range
            if (m_eventNr >= eventTrack->GetNumEvents())
            {
                return AZ::Failure();
            }

            // get the motion event
            return AZ::Success(&eventTrack->GetEvent(m_eventNr));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // add event track
    void CommandAddEventTrack(EMotionFX::Motion* motion)
    {
        // make sure the motion is valid
        if (motion == nullptr)
        {
            return;
        }

        EMotionFX::MotionEventTable* motionEventTable = motion->GetEventTable();
        size_t trackNr = motionEventTable->GetNumTracks() + 1;

        AZStd::string eventTrackName;
        eventTrackName = AZStd::string::format("Event Track %zu", trackNr);
        while (motionEventTable->FindTrackByName(eventTrackName.c_str()))
        {
            ++trackNr;
            eventTrackName = AZStd::string::format("Event Track %zu", trackNr);
        }

        AZStd::string outResult;
        AZStd::string command = AZStd::string::format("CreateMotionEventTrack -motionID %i -eventTrackName \"%s\"", motion->GetID(), eventTrackName.c_str());
        if (GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }


    // add event track
    void CommandAddEventTrack()
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        CommandAddEventTrack(motion);
    }


    // remove event track
    void CommandRemoveEventTrack(EMotionFX::Motion* motion, size_t trackIndex)
    {
        if (!motion)
        {
            return;
        }

        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();
        EMotionFX::MotionEventTrack* eventTrack = eventTable->GetTrack(trackIndex);

        AZStd::string outResult;
        MCore::CommandGroup commandGroup("Remove event track");

        const size_t numMotionEvents = eventTrack->GetNumEvents();
        for (size_t j = 0; j < numMotionEvents; ++j)
        {
            commandGroup.AddCommandString(AZStd::string::format("RemoveMotionEvent -motionID %i -eventTrackName \"%s\" -eventNr %i", motion->GetID(), eventTrack->GetName(), 0).c_str());
        }

        commandGroup.AddCommandString(AZStd::string::format("RemoveMotionEventTrack -motionID %i -eventTrackName \"%s\"", motion->GetID(), eventTrack->GetName()).c_str());

        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult))
        {
            AZ_Error("EMotionFX", !outResult.empty(), outResult.c_str());
        }
    }


    // remove event track
    void CommandRemoveEventTrack(size_t trackIndex)
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        CommandRemoveEventTrack(motion, trackIndex);
    }


    // rename event track
    void CommandRenameEventTrack(EMotionFX::Motion* motion, size_t trackIndex, const char* newName)
    {
        // make sure the motion is valid
        if (motion == nullptr)
        {
            return;
        }

        // get the motion event table and the given event track
        EMotionFX::MotionEventTable*    eventTable  = motion->GetEventTable();
        EMotionFX::MotionEventTrack*    eventTrack  = eventTable->GetTrack(trackIndex);

        AZStd::string outResult;
        AZStd::string command = AZStd::string::format("AdjustMotionEventTrack -motionID %i -eventTrackName \"%s\" -newName \"%s\"", motion->GetID(), eventTrack->GetName(), newName);
        if (GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }


    // rename event track
    void CommandRenameEventTrack(size_t trackIndex, const char* newName)
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        CommandRenameEventTrack(motion, trackIndex, newName);
    }


    // enable or disable event track
    void CommandEnableEventTrack(EMotionFX::Motion* motion, size_t trackIndex, bool isEnabled)
    {
        // make sure the motion is valid
        if (motion == nullptr)
        {
            return;
        }

        // get the motion event table and the given event track
        EMotionFX::MotionEventTable*    eventTable  = motion->GetEventTable();
        EMotionFX::MotionEventTrack*    eventTrack  = eventTable->GetTrack(trackIndex);

        AZStd::string outResult;
        AZStd::string command = AZStd::string::format("AdjustMotionEventTrack -motionID %i -eventTrackName \"%s\" -enabled %s", 
            motion->GetID(), 
            eventTrack->GetName(), 
            AZStd::to_string(isEnabled).c_str());
        if (GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }


    // enable or disable event track
    void CommandEnableEventTrack(size_t trackIndex, bool isEnabled)
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        CommandEnableEventTrack(motion, trackIndex, isEnabled);
    }


    // add a new motion event
    void CommandHelperAddMotionEvent(const char* trackName, float startTime, float endTime, const EMotionFX::EventDataSet& eventData, MCore::CommandGroup* commandGroup)
    {
        const EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        CommandHelperAddMotionEvent(motion, trackName, startTime, endTime, eventData, commandGroup);
    }


    // remove motion event
    void CommandHelperRemoveMotionEvent(EMotionFX::Motion* motion, const char* trackName, size_t eventNr, MCore::CommandGroup* commandGroup)
    {
        // make sure the motion is valid
        if (motion == nullptr)
        {
            return;
        }

        // create our command group
        MCore::CommandGroup internalCommandGroup("Remove motion event");

        // execute the create motion event command
        AZStd::string command;
        command = AZStd::string::format("RemoveMotionEvent -motionID %i -eventTrackName \"%s\" -eventNr %zu", motion->GetID(), trackName, eventNr);

        // add the command to the command group
        if (commandGroup == nullptr)
        {
            internalCommandGroup.AddCommandString(command.c_str());
        }
        else
        {
            commandGroup->AddCommandString(command.c_str());
        }

        // execute the group command
        if (commandGroup == nullptr)
        {
            AZStd::string outResult;
            if (GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, outResult) == false)
            {
                MCore::LogError(outResult.c_str());
            }
        }
    }


    // remove motion event
    void CommandHelperRemoveMotionEvent(uint32 motionID, const char* trackName, size_t eventNr, MCore::CommandGroup* commandGroup)
    {
        // find the motion by id
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (motion == nullptr)
        {
            return;
        }

        CommandHelperRemoveMotionEvent(motion, trackName, eventNr, commandGroup);
    }

    // remove motion event
    void CommandHelperRemoveMotionEvent(const char* trackName, size_t eventNr, MCore::CommandGroup* commandGroup)
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        if (motion == nullptr)
        {
            return;
        }

        CommandHelperRemoveMotionEvent(motion, trackName, eventNr, commandGroup);
    }


    // remove motion event
    void CommandHelperRemoveMotionEvents(uint32 motionID, const char* trackName, const AZStd::vector<size_t>& eventNumbers, MCore::CommandGroup* commandGroup)
    {
        // find the motion by id
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (motion == nullptr)
        {
            return;
        }

        // create our command group
        MCore::CommandGroup internalCommandGroup("Remove motion events");

        // get the number of events to remove and iterate through them
        const size_t numEvents = eventNumbers.size();
        for (size_t i = 0; i < numEvents; ++i)
        {
            // remove the events from back to front
            size_t eventNr = eventNumbers[numEvents - 1 - i];

            // add the command to the command group
            if (commandGroup == nullptr)
            {
                CommandHelperRemoveMotionEvent(motion, trackName, eventNr, &internalCommandGroup);
            }
            else
            {
                CommandHelperRemoveMotionEvent(motion, trackName, eventNr, commandGroup);
            }
        }

        // execute the group command
        if (commandGroup == nullptr)
        {
            AZStd::string outResult;
            if (GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, outResult) == false)
            {
                MCore::LogError(outResult.c_str());
            }
        }
    }


    // remove motion event
    void CommandHelperRemoveMotionEvents(const char* trackName, const AZStd::vector<size_t>& eventNumbers, MCore::CommandGroup* commandGroup)
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        if (motion == nullptr)
        {
            return;
        }

        CommandHelperRemoveMotionEvents(motion->GetID(), trackName, eventNumbers, commandGroup);
    }


    void CommandHelperMotionEventTrackChanged(EMotionFX::Motion* motion, size_t eventNr, float startTime, float endTime, const char* oldTrackName, const char* newTrackName)
    {
        // get the motion event track
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(oldTrackName);
        if (eventTrack == nullptr)
        {
            return;
        }

        // check if the motion event number is valid and return in case it is not
        if (eventNr >= eventTrack->GetNumEvents())
        {
            return;
        }

        // create the command group
        AZStd::string result;
        MCore::CommandGroup commandGroup("Change motion event track");

        // get the motion event
        EMotionFX::MotionEvent& motionEvent = eventTrack->GetEvent(eventNr);

        commandGroup.AddCommandString(AZStd::string::format("RemoveMotionEvent -motionID %i -eventTrackName \"%s\" -eventNr %zu", motion->GetID(), oldTrackName, eventNr));
        CommandHelperAddMotionEvent(motion, newTrackName, startTime, endTime, motionEvent.GetEventDatas(), &commandGroup);

        // execute the command group
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, result) == false)
        {
            MCore::LogError(result.c_str());
        }
    }


    void CommandHelperMotionEventTrackChanged(size_t eventNr, float startTime, float endTime, const char* oldTrackName, const char* newTrackName)
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        CommandHelperMotionEventTrackChanged(motion, eventNr, startTime, endTime, oldTrackName, newTrackName);
    }
} // namespace CommandSystem
