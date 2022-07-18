/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Include/ScriptCanvas/Libraries/Core/AzEventHandler.generated.h>

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Node.h>

namespace ScriptCanvas::Nodes::Core
{
    class AzEventEntrySerializer;

    struct AzEventEntry
    {
        static void Reflect(AZ::ReflectContext* context);

        AZ_TYPE_INFO(AzEventEntry, "{8DAD77FB-9A98-4E31-A714-999A342C2B31}");

        AZStd::string m_eventName;
        AZStd::vector<SlotId> m_parameterSlotIds;
        AZStd::vector<AZStd::string> m_parameterNames;
        SlotId m_azEventInputSlotId;
    };

    class AzEventHandler
        : public Node
    {
        friend class AzEventEntrySerializer;

    public:
        SCRIPTCANVAS_NODE(AzEventHandler);
        
        AzEventHandler() = default;
        AzEventHandler(const AZ::BehaviorMethod& methodWithAzEventReturn);
        ~AzEventHandler() override;

        //! Initializes the AzEventHandler a BehaviorContextMethod node which returns an AZ::Event
        //! by reference or by pointer
        bool InitEventFromMethod(const AZ::BehaviorMethod& methodWhichReturnsEvent);

        //! Set the node ID for the Restricted Node Contract on the Connect and AzEvent data input slot
        //! Only the BehaviorMethod ScriptCanvas node that this AzEventHandler was created from
        //! can connect to this event handler
        void SetRestrictedNodeId(AZ::EntityId methodNodeId);

        void CollectVariableReferences(AZStd::unordered_set<ScriptCanvas::VariableId>& variableIds) const override;
        bool ContainsReferencesToVariables(const AZStd::unordered_set<ScriptCanvas::VariableId>& variableIds) const override;

        size_t GenerateFingerprint() const override;        

        AZ::Outcome<Grammar::LexicalScope, void> GetFunctionCallLexicalScope(const Slot* slot) const override;

        AZ::Outcome<AZStd::string, void> GetFunctionCallName(const Slot* slot) const override;

        const AzEventEntry& GetEventEntry() const;
        const Slot* GetEventInputSlot() const;
        AZStd::vector<SlotId> GetEventSlotIds() const override;
        AZ::Outcome<AZStd::string> GetInternalOutKey(const Slot& slot) const override;
        AZStd::vector<SlotId> GetNonEventSlotIds() const override;

        AZ::Outcome<DependencyReport, void> GetDependencies() const override;

        bool IsEventSlotId(const SlotId& slotId) const;

        bool IsEventHandler() const override;

        AZStd::string GetNodeName() const override;
        AZStd::string GetDebugName() const override;

        NodeTypeIdentifier GetOutputNodeType(const SlotId& slotId) const override;

    protected:
        ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* executionChildSlot) const override;

    private:
        AzEventEntry m_azEventEntry;
    };
}
