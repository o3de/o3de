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

#pragma once

#include <Include/ScriptCanvas/Libraries/Core/AzEventHandler.generated.h>

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Node.h>

namespace ScriptCanvas::Nodes::Core
{
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

        bool IsSupportedByNewBackend() const override { return true; }

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
        SlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* executionChildSlot) const override;

    private:
        AzEventEntry m_azEventEntry;
    };
}
