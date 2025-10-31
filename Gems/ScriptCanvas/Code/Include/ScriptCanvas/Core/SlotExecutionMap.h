/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Outcome/Outcome.h>

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Variable/VariableCore.h>

#include "SubgraphInterface.h"

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    namespace SlotExecution
    {
        // Represents a data slot output
        struct Output final
        {
            AZ_TYPE_INFO(Output, "{61EA2FF0-3112-40DF-BA45-CF4BE680DC52}");
            AZ_CLASS_ALLOCATOR(Output, AZ::SystemAllocator);

            SlotId slotId;
            VariableId interfaceSourceId;

            Output() = default;
            // not explicit on purpose
            Output(const SlotId& slotId);
            Output(const SlotId& slotId, const VariableId& interfaceSourceId);
        };

        using Outputs = AZStd::vector<Output>;
        using OutputSlotIds = AZStd::vector<SlotId>;

        // Represents a data slot input
        struct Input final
        {
            AZ_TYPE_INFO(Input, "{4E52A04D-C9FC-477F-8065-35F96A972CD6}");
            AZ_CLASS_ALLOCATOR(Input, AZ::SystemAllocator);

            SlotId slotId;
            VariableId interfaceSourceId;

            Input() = default;
            // not explicit on purpose
            Input(const SlotId& slotId);
            Input(const SlotId& slotId, const VariableId & interfaceSourceId);
        };
        using Inputs = AZStd::vector<Input>;
        using InputSlotIds = AZStd::vector<SlotId>;

        struct Return final
        {
            AZ_TYPE_INFO(Return, "{8CD09346-BF99-4B34-91EA-C553549F7639}");
            AZ_CLASS_ALLOCATOR(Return, AZ::SystemAllocator);

            Inputs values;
        };

        // Represents an execution slot output
        struct Out final
        {
            AZ_TYPE_INFO(Out, "{DD3D2547-868C-40DF-A37C-F60BE06FFFBA}");
            AZ_CLASS_ALLOCATOR(Out, AZ::SystemAllocator);

            SlotId slotId;
            AZStd::string name;
            Outputs outputs;
            Return returnValues;
            Grammar::FunctionSourceId interfaceSourceId;
        };
        using Outs = AZStd::vector<Out>;

        // Represents an execution slot input
        struct In final
        {
            AZ_TYPE_INFO(In, "{4AAAEB0B-6367-46E5-B05D-E76EF884E16F}");
            AZ_CLASS_ALLOCATOR(In, AZ::SystemAllocator);

            SlotId slotId;
            Inputs inputs;
            Outs outs;
            AZStd::string parsedName;
            Grammar::FunctionSourceId interfaceSourceId;

            AZ_INLINE bool IsBranch() const { return outs.size() > 1; }
        };
        using Ins = AZStd::vector<In>;

        InputSlotIds ToInputSlotIds(const Inputs& source);
        OutputSlotIds ToOutputSlotIds(const Outputs& source);

        // Maps slots of nodes to one another to indicate what execution slots corrospond to what data slots
        class Map final
        {
        public:
            AZ_TYPE_INFO(Map, "{BAA81EAF-E35A-4F19-B73A-699B91DB113C}");
            AZ_CLASS_ALLOCATOR(Map, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* refectContext);

            Map() = default;

            Map(Ins&& ins);

            Map(Ins&& ins, Outs&& latents);

            Map(Outs&& latents);

            // Takes in a data input slot id, and returns the execution in associated with it
            const In* FindInFromInputSlot(const SlotId& slotID) const;

            // Takes in a data output slot id, and returns the execution out associated with it
            const Out* FindOutFromOutputSlot(const SlotId& slotID) const;

            SlotId FindInputSlotIdBySource(VariableId inputSourceId, Grammar::FunctionSourceId inSourceId) const;

            SlotId FindInSlotIdBySource(Grammar::FunctionSourceId sourceId) const;

            SlotId FindLatentSlotIdBySource(Grammar::FunctionSourceId sourceId) const;

            SlotId FindOutputSlotIdBySource(VariableId sourceId) const;

            SlotId FindOutSlotIdBySource(Grammar::FunctionSourceId inSourceID, Grammar::FunctionSourceId outSourceId) const;

            const In* GetIn(size_t index) const;

            const In* GetIn(SlotId in) const;

            const Ins& GetIns() const;

            // Takes in the slot ID of an execution in slot and returns a vector of its corresponding data inputs
            const Inputs* GetInput(SlotId in) const;

            const Out* GetLatent(SlotId latent) const;

            const Outputs* GetLatentOutput(SlotId latent) const;

            const Outs& GetLatents() const;

            const Out* GetOut(SlotId out) const;

            const Out* GetOut(SlotId in, SlotId out) const;

            // Takes in the slot ID of an execution out slot and returns a vector of its corresponding data outputs
            const Outputs* GetOutput(SlotId out) const;

            const Outputs* GetOutput(SlotId in, SlotId out) const;

            const Outs* GetOuts(SlotId in) const;

            const Inputs* GetReturnValues(SlotId inSlotId, SlotId outSlotId) const;

            const Inputs* GetReturnValuesByOut(SlotId in) const;

            AZ::Outcome<bool> IsBranch(SlotId in) const;

            bool IsEmpty() const;

            // Returns true if there is at least one latent out
            bool IsLatent() const;

            AZStd::string ToExecutionString() const;


        private:
            Ins m_ins;
            Outs m_latents;

            const Out* FindImmediateOut(SlotId out, bool errorOnFailure = true) const;
            const Out* FindImmediateOut(SlotId in, SlotId out, bool errorOnFailure = true) const;
            const In* FindInFromSlotId(SlotId in) const;
            const Out* FindLatentOut(SlotId latent, bool errorOnFailure = true) const;
        };
        
    }
} 
