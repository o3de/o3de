/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/EventData.h>
#include <EMotionFX/Source/Event.h>
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>


namespace AZ
{
    class ReflectContext;
}

namespace CommandSystem
{

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // EMotionFX::Motion* Event Track
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    class DEFINECOMMAND_API CommandCreateMotionEventTrack
        : public MCore::Command, public MotionIdCommandMixin
    {
    public:
        AZ_RTTI(CommandSystem::CommandCreateMotionEventTrack, "{961F762D-5B90-4E21-8692-9FADDCA54E6C}", MCore::Command, MotionIdCommandMixin)
        AZ_CLASS_ALLOCATOR_DECL

        CommandCreateMotionEventTrack(MCore::Command* orgCommand = nullptr);

        static void Reflect(AZ::ReflectContext* context);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;
        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Create motion event track"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandCreateMotionEventTrack(this); }

        void SetEventTrackName(const AZStd::string& newName) { m_eventTrackName = newName; }

    private:
        AZStd::string m_eventTrackName;
        AZStd::optional<size_t> m_eventTrackIndex;
        AZStd::optional<bool> m_isEnabled;
    };

    MCORE_DEFINECOMMAND_START(CommandRemoveMotionEventTrack, "Remove motion event track", true)
    size_t          m_oldTrackIndex;
    bool            m_oldEnabled;
    MCORE_DEFINECOMMAND_END

    class DEFINECOMMAND_API CommandAdjustMotionEventTrack
        : public MCore::Command, public MotionIdCommandMixin
    {
    public:
        AZ_RTTI(CommandSystem::CommandAdjustMotionEventTrack, "{B38FB511-B820-4F7C-9857-314DFCCE4E9A}", MCore::Command, MotionIdCommandMixin)
        AZ_CLASS_ALLOCATOR_DECL

        CommandAdjustMotionEventTrack(MCore::Command* orgCommand = nullptr);

        static void Reflect(AZ::ReflectContext* context);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;
        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Adjust motion event track"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandAdjustMotionEventTrack(this); }

        void SetEventTrackName(const AZStd::string& newName) { m_eventTrackName = newName; }
        void SetNewName(const AZStd::string& newName) { m_newName = newName; }
        void SetIsEnabled(const bool newIsEnabled) { m_isEnabled = newIsEnabled; }

    private:
        AZStd::string m_eventTrackName;
        AZStd::optional<AZStd::string> m_newName;
        AZStd::string m_oldName;
        AZStd::optional<bool> m_isEnabled;
        bool m_oldEnabled;
    };

    class DEFINECOMMAND_API CommandClearMotionEvents
        : public MCore::Command, public MotionIdCommandMixin
    {
    public:
        AZ_RTTI(CommandSystem::CommandClearMotionEvents, "{65A5556C-B7FF-4379-86DA-AD8642398079}", MCore::Command, MotionIdCommandMixin)
        AZ_CLASS_ALLOCATOR_DECL

        CommandClearMotionEvents(MCore::Command * orgCommand = nullptr);

        static void Reflect(AZ::ReflectContext* context);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;
        bool GetIsUndoable() const override { return false; }
        const char* GetHistoryName() const override { return "Clear all motion events"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandClearMotionEvents(this); }
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // EMotionFX::Motion* Events
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    class DEFINECOMMAND_API CommandCreateMotionEvent
        : public MCore::Command, public MotionIdCommandMixin
    {
    public:
        AZ_RTTI(CommandSystem::CommandCreateMotionEvent, "{D19C2AFB-A5AA-4367-BCFC-02EB88C1B61F}", MCore::Command, MotionIdCommandMixin)
        AZ_CLASS_ALLOCATOR_DECL

        CommandCreateMotionEvent(MCore::Command* orgCommand = nullptr);

        static void Reflect(AZ::ReflectContext* context);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;
        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Create motion event"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandCreateMotionEvent(this); }

        void SetEventTrackName(const AZStd::string& newName) { m_eventTrackName = newName; }
        void SetStartTime(float newStartTime) { m_startTime = newStartTime; }
        void SetEndTime(float newEndTime) { m_endTime = newEndTime; }
        void SetEventDatas(const EMotionFX::EventDataSet& newData) { m_eventDatas = newData; }

    public:
        AZStd::string m_eventTrackName;
        AZStd::optional<AZStd::string> m_serializedEventData;
        AZStd::optional<EMotionFX::EventDataSet> m_eventDatas;
        float m_startTime = 0.0f;
        float m_endTime = 0.0f;
        size_t m_motionEventNr;
    };

        MCORE_DEFINECOMMAND_START(CommandRemoveMotionEvent, "Remove motion event", true)
    float           m_oldStartTime;
    float           m_oldEndTime;
    EMotionFX::EventDataSet m_oldEventDatas;
    MCORE_DEFINECOMMAND_END

    class DEFINECOMMAND_API CommandAdjustMotionEvent
        : public MCore::Command, public MotionIdCommandMixin
    {
    public:
        AZ_RTTI(CommandSystem::CommandAdjustMotionEvent, "{D175BD8D-674E-463A-AFCE-22EBE7A56D0F}", MCore::Command, MotionIdCommandMixin)
        AZ_CLASS_ALLOCATOR_DECL

        enum class EventDataAction : AZ::u8
        {
            None,
            Replace,
            Add,
            Remove
        };

        CommandAdjustMotionEvent(MCore::Command* orgCommand = nullptr);

        static void Reflect(AZ::ReflectContext* context);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;
        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Adjust motion event"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandAdjustMotionEvent(this); }

        void SetEventTrackName(const AZStd::string& newName) { m_eventTrackName = newName; }
        void SetEventNr(size_t eventNr) { m_eventNr = eventNr; }
        void SetEventDataNr(size_t eventDataNr) { m_eventDataNr = eventDataNr; }
        void SetEventData(EMotionFX::EventDataPtr&& eventData) { m_eventData = AZStd::move(eventData); }
        void SetEventDataAction(EventDataAction action) { m_eventDataAction = action; }

        AZ::Outcome<EMotionFX::MotionEvent*> GetMotionEvent() const;
        void SetMotionEvent(EMotionFX::MotionEvent* newEvent) { m_motionEvent = newEvent; }

    private:
        AZStd::string m_eventTrackName;
        size_t m_eventNr;
        size_t m_eventDataNr;
        float m_oldStartTime;
        float m_oldEndTime;
        AZStd::optional<float> m_startTime;
        AZStd::optional<float> m_endTime;
        AZStd::optional<EMotionFX::EventDataPtr> m_eventData;
        EMotionFX::EventDataPtr m_oldEventData;
        EMotionFX::MotionEvent* m_motionEvent = nullptr;
        EventDataAction m_eventDataAction = EventDataAction::None;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Command helpers
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void COMMANDSYSTEM_API CommandAddEventTrack();
    void COMMANDSYSTEM_API CommandRemoveEventTrack(size_t trackIndex);
    void COMMANDSYSTEM_API CommandRemoveEventTrack(EMotionFX::Motion* motion, size_t trackIndex);
    void COMMANDSYSTEM_API CommandRenameEventTrack(size_t trackIndex, const char* newName);
    void COMMANDSYSTEM_API CommandEnableEventTrack(size_t trackIndex, bool isEnabled);
    void COMMANDSYSTEM_API CommandHelperAddMotionEvent(const char* trackName, float startTime, float endTime, const EMotionFX::EventDataSet& eventDatas = EMotionFX::EventDataSet {}, MCore::CommandGroup* commandGroup = nullptr);
    void COMMANDSYSTEM_API CommandHelperRemoveMotionEvent(const char* trackName, size_t eventNr, MCore::CommandGroup* commandGroup = nullptr);
    void COMMANDSYSTEM_API CommandHelperRemoveMotionEvent(uint32 motionID, const char* trackName, size_t eventNr, MCore::CommandGroup* commandGroup = nullptr);
    void COMMANDSYSTEM_API CommandHelperRemoveMotionEvents(const char* trackName, const AZStd::vector<size_t>& eventNumbers, MCore::CommandGroup* commandGroup = nullptr);
    void COMMANDSYSTEM_API CommandHelperMotionEventTrackChanged(size_t eventNr, float startTime, float endTime, const char* oldTrackName, const char* newTrackName);
} // namespace CommandSystem
