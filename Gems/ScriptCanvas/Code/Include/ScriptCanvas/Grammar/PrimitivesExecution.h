
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Primitives.h"

namespace ScriptCanvas
{
    using EndpointResolved = AZStd::pair<const Node*, const Slot*>;
    using EndpointsResolved = AZStd::vector<EndpointResolved>;

    namespace Grammar
    {
        // The node and the activation slot. The execution in, or the event or latent out slot.
        struct ExecutionId
        {
            AZ_CLASS_ALLOCATOR(ExecutionId, AZ::SystemAllocator);

            const Node* m_node = nullptr;
            const Slot* m_slot = nullptr;

            AZ_INLINE bool operator==(const ExecutionId& rhs) const { return m_node == rhs.m_node && m_slot == rhs.m_slot; }
            AZ_INLINE bool operator!=(const ExecutionId& rhs) const { return m_node != rhs.m_node || m_slot != rhs.m_slot; };

            ExecutionId() = default;

            AZ_INLINE ExecutionId(const Node* node, const Slot* slot)
                : m_node(node)
                , m_slot(slot)
            {}
        };

    } 

} 

namespace AZStd
{
    template<>
    struct hash<ScriptCanvas::Grammar::ExecutionId>
    {
        using argument_type = ScriptCanvas::Grammar::ExecutionId;
        using result_type = size_t;

        AZ_INLINE result_type operator() (const argument_type& id) const
        {
            size_t h = 0;
            hash_combine(h, id.m_node);
            hash_combine(h, id.m_slot);
            return h;
        }
    };

}

namespace ScriptCanvas
{
    namespace Grammar
    {
        // The node and the activation slot. The execution in, or the event or latent out slot.

        class ExecutionTreeTraversalListener;

        struct ExecutionInput
        {
            AZ_TYPE_INFO(ExecutionInput, "{103413DF-830E-418F-A5CB-645063F1D93F}");
            AZ_CLASS_ALLOCATOR(ExecutionInput, AZ::SystemAllocator);

            const Slot* m_slot = nullptr;
            VariableConstPtr m_value;
            DebugDataSource m_sourceDebug;
        };

        struct ExecutionChild
        {
            AZ_TYPE_INFO(ExecutionChild, "{29966A61-D7E3-4491-A14B-12DDF65D61D2}");
            AZ_CLASS_ALLOCATOR(ExecutionChild, AZ::SystemAllocator);

            // can be nullptr if a single graph node is split into multiple grammar nodes
            const Slot* m_slot = nullptr;
            // will always be valid
            AZStd::vector<AZStd::pair<const Slot*, OutputAssignmentConstPtr>> m_output;
            // can be nullptr if no execution continues
            ExecutionTreePtr m_execution;
        };

        struct ExecutionTree
            : public AZStd::enable_shared_from_this<ExecutionTree>
        {
            AZ_TYPE_INFO(ExecutionTree, "{B062AFDC-7BC7-415B-BFC4-EFEE8D1CE87A}");
            AZ_CLASS_ALLOCATOR(ExecutionTree, AZ::SystemAllocator);

            ~ExecutionTree();

            void AddChild(const ExecutionChild& child);

            void AddInput(const ExecutionInput& input);

            void AddReturnValue(const Slot* slot, ReturnValueConstPtr returnValue);

            void AddPropertyExtractionSource(const Slot* slot, PropertyExtractionConstPtr propertyExtraction);

            void Clear();

            void ClearChildren();

            void ClearInput();

            void ClearProperyExtractionSources();

            void ConvertNameToIdentifier();

            enum class RemapVariableSource
            {
                No,
                Yes
            };
            void CopyInput(ExecutionTreeConstPtr source, RemapVariableSource remapSource);

            void CopyReturnValuesToInputs(ExecutionTreeConstPtr source);

            ExecutionChild* FindChild(const SlotId slotId);

            const ExecutionChild* FindChildConst(ExecutionTreeConstPtr execution) const;

            const ExecutionChild* FindChildConst(const SlotId slotId) const;

            size_t FindChildIndex(ExecutionTreeConstPtr execution) const;

            const ExecutionChild& GetChild(size_t index) const;

            size_t GetChildrenCount() const;

            const ConversionByIndex& GetConversions() const;

            PropertyExtractionConstPtr GetExecutedPropertyExtraction() const;

            EventType GetEventType() const;

            const ExecutionId& GetId() const;

            AZ::EntityId GetNodeId() const;

            const ExecutionInput& GetInput(size_t index) const;

            size_t GetInputCount() const;

            AZStd::vector<ExecutionTreeConstPtr> GetInternalOuts() const;

            // if there's only one child, returns its output, all other cases must be handled by nodeable
            // out translation
            const AZStd::vector<AZStd::pair<const Slot*, OutputAssignmentConstPtr>>* GetLocalOutput() const;

            MetaDataConstPtr GetMetaData() const;

            const AZStd::any& GetMetaDataEx() const;

            const AZStd::string& GetName() const;

            const LexicalScope& GetNameLexicalScope() const;

            VariableConstPtr GetNodeable() const;

            AZStd::optional<size_t> GetOutCallIndex() const;

            ExecutionTreeConstPtr GetParent() const;

            ExecutionTreeConstPtr GetRoot() const;

            const AZStd::vector<AZStd::pair<const Slot*, PropertyExtractionConstPtr>>& GetPropertyExtractionSources() const;

            AZStd::pair<const Slot*, ReturnValueConstPtr> GetReturnValue(size_t index) const;

            size_t GetReturnValueCount() const;

            ScopeConstPtr GetScope() const;

            Symbol GetSymbol() const;

            bool HasExplicitUserOutCalls() const;

            bool HasReturnValues() const;

            bool InputHasThisPointer() const;

            bool IsInfiniteLoopDetectionPoint() const;

            void InsertChild(size_t index, const ExecutionChild& child);

            bool IsInputOutputPreprocessed() const;

            bool IsInternalOut() const;

            bool IsOnLatentPath() const;

            bool IsPure() const;

            bool IsStartCall() const;

            void MarkDebugEmptyStatement();

            void MarkHasExplicitUserOutCalls();

            void MarkInfiniteLoopDetectionPoint();

            void MarkInputHasThisPointer();

            void MarkInputOutputPreprocessed();

            void MarkInternalOut();

            void MarkPure();

            void MarkRefersToSelfEntityId();

            void MarkRootLatent();

            void MarkStartCall();

            ExecutionChild& ModChild(size_t index);

            ConversionByIndex& ModConversions();

            ExecutionInput& ModInput(size_t index);

            MetaDataPtr ModMetaData();

            AZStd::any& ModMetaDataEx();

            ExecutionTreePtr ModParent();

            ExecutionTreePtr ModRoot();

            ScopePtr ModScope();

            ScopePtr ModScopeFunction();

            void ReduceInputSet(const AZ::InputRestriction& indices);

            bool RefersToSelfEntityId() const;

            AZ::Outcome<AZStd::pair<size_t, ExecutionChild>> RemoveChild(const ExecutionTreeConstPtr& child);

            void SetExecutedPropertyExtraction(PropertyExtractionConstPtr propertyExtraction);

            void SetEventType(EventType eventType);

            void SetId(const ExecutionId& id);

            void SetMetaData(MetaDataPtr metaData);

            void SetName(AZStd::string_view name);

            void SetNameLexicalScope(const LexicalScope& lexicalScope);

            void SetNodeable(VariableConstPtr nodeable);

            void SetOutCallIndex(size_t index);

            void SetParent(ExecutionTreePtr parent);

            void SetScope(ScopePtr scope);

            void SetSymbol(Symbol val);

            void SwapChildren(ExecutionTreePtr execution);

        private:
            // the (possible) slot(s) through which execution exited, along with associated output
            AZStd::vector<ExecutionChild> m_children;

            EventType m_eventType = EventType::Count;

            // input to execution, regardless of source slot type, optionally keyed by slot
            AZStd::vector<ExecutionInput> m_input;

            ConversionByIndex m_inputConversion;

            bool m_hasExplicitUserOutCalls = false;

            bool m_isInfiniteLoopDetectionPoint = false;

            bool m_inputHasThisPointer = false;

            bool m_isInputOutputPreprocessed = false;

            bool m_isInternalOut = false;

            bool m_isLatent = false;

            bool m_isPure = false;

            bool m_isStartCall = false;

            // #scriptcanvas_component_extension
            bool m_refersToSelfEntityId = false;

            size_t m_outCallIndex = std::numeric_limits<size_t>::max();

            // The node and the activation slot. The execution in, or the event or latent out slot.
            ExecutionId m_in;

            AZStd::any m_metaDataEx;

            MetaDataPtr m_metaData;
            
            AZStd::string m_name;

            LexicalScope m_lexicalScope;

            ExecutionTreePtr m_parent;

            // \todo consider moving to meta data
            PropertyExtractionConstPtr m_propertyExtractionExecuted;

            // \todo remove, is this is temporary parsing data
            AZStd::vector<AZStd::pair<const Slot*, PropertyExtractionConstPtr>> m_propertyExtractionsSource;
            
            // optional return values, currently for events only
            AZStd::vector<AZStd::pair<const Slot*, ReturnValueConstPtr>> m_returnValues;

            ScopePtr m_scope;

            Symbol m_symbol = Symbol::FunctionCall;

            VariableConstPtr m_nodeable;

            size_t FindIndexOfChild(ExecutionTreeConstPtr child) const;
        };

        class ExecutionTreeTraversalListener
        {
        public:
            virtual ~ExecutionTreeTraversalListener() {}

            virtual bool CancelledTraversal() { return false; }
            virtual void Evaluate(ExecutionTreeConstPtr /*node*/, const Slot* /*slot*/, int /*level*/) {}
            virtual void EvaluateNullChildLeaf(ExecutionTreeConstPtr /*parent*/, const Slot* /*slot*/, size_t /*index*/, int /*level*/) {}
            virtual void EvaluateChildPost(ExecutionTreeConstPtr /*node*/, const Slot* /*slot*/, size_t /*index*/, int /*level*/) {}
            virtual void EvaluateChildPre(ExecutionTreeConstPtr /*node*/, const Slot* /*slot*/, size_t /*index*/, int /*level*/) {}
            virtual void EvaluateRoot(ExecutionTreeConstPtr /*node*/, const Slot* /*slot*/) {}
            virtual void EvaluateLeaf(ExecutionTreeConstPtr /*node*/, const Slot* /*slot*/, int /*level*/) {}
            virtual void Reset() {}
        };

        enum class ExecutionTraversalResult
        {
            Success,
            ContainsCycle,
            NullSlot,
            NullNode,
            GetSlotError,
        };

        class GraphExecutionPathTraversalListener
        {
        public:
            virtual ~GraphExecutionPathTraversalListener() {}

            virtual bool CancelledTraversal() { return false; }
            virtual void Evaluate([[maybe_unused]] const EndpointResolved& endpoint) {}
        };
    }
} 
