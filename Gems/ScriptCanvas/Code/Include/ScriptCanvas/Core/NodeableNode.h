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

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/SlotExecutionMap.h>

namespace AZ
{
    class BehaviorContext;
}

namespace ScriptCanvas
{
    namespace Grammar
    {
        struct Out;
    }

    namespace Nodes
    {
        class NodeableNode
            : public Node
        {
        public:
            static const AZ::Crc32 c_onVariableHandlingGroup;

            AZ_COMPONENT(NodeableNode, "{80351020-5778-491A-B6CA-C78364C19499}", Node);
            static void Reflect(AZ::ReflectContext* reflectContext);

            AZ::Outcome<DependencyReport, void> GetDependencies() const override;
            
            AZ::Outcome<AZStd::string, void> GetFunctionCallName(const Slot* /*slot*/) const override;

            AZ::Outcome<Grammar::LexicalScope, void> GetFunctionCallLexicalScope(const Slot* /*slot*/) const override;

            Nodeable* GetMutableNodeable() const;

            const Nodeable* GetNodeable() const;

            AZ::TypeId GetNodeableType() const;

            AZStd::vector<const Slot*> GetOnVariableHandlingDataSlots() const override;

            AZStd::vector<const Slot*> GetOnVariableHandlingExecutionSlots() const override;

            NodePropertyInterface* GetPropertyInterface(AZ::Crc32 propertyId) override;

            const SlotExecution::Map* GetSlotExecutionMap() const override;

            bool IsNodeableNode() const override;            

            Nodeable* ReleaseNodeable();

            void SetNodeable(AZStd::unique_ptr<Nodeable> nodeable);

            void SetSlotExecutionMap(SlotExecution::Map&& map);

        protected:
            virtual void AddOut(const Grammar::Out& out, const AZStd::string& name, const AZStd::string& dataPrefix, bool isLatent, SlotExecution::Outs& outs);
            
            void ConfigureSlots() override;

            AZ::Outcome<const AZ::BehaviorClass*, AZStd::string> GetBehaviorContextClass() const;

            ConstSlotsOutcome GetBehaviorContextOutName(const Slot& inSlot) const;

            virtual void RegisterExecutionMap(const AZ::BehaviorContext&) {}          

            AZStd::unique_ptr<Nodeable> m_nodeable;

            SlotExecution::Map m_slotExecutionMap;

        };
    } 
}

