/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PrimitivesExecution.h"

#include <ScriptCanvas/Core/EBusHandler.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/Grammar/DebugMap.h>
#include <ScriptCanvas/Translation/Configuration.h>
#include <ScriptCanvas/Variable/VariableData.h>

#include "ParsingUtilities.h"

namespace ScriptCanvas
{
    namespace Grammar
    {
        ExecutionTree::~ExecutionTree()
        {
            Clear();
        }

        void ExecutionTree::AddChild(const ExecutionChild& child)
        {
            m_children.push_back(child);
        }

        void ExecutionTree::AddInput(const ExecutionInput& input)
        {
            if (!RefersToSelfEntityId() && IsInputSelf(input))
            {
                MarkRefersToSelfEntityId();
            }

            m_input.push_back(input);
        }

        void ExecutionTree::AddPropertyExtractionSource(const Slot* slot, PropertyExtractionConstPtr propertyExtraction)
        {
            m_propertyExtractionsSource.push_back({ slot, propertyExtraction });
        }

        void ExecutionTree::AddReturnValue(const Slot* slot, ReturnValueConstPtr returnValue)
        {
            m_returnValues.emplace_back(slot, returnValue);
        }

        void ExecutionTree::Clear()
        {
            m_nodeable = nullptr;
            m_parent = nullptr;
            m_metaData = nullptr;

            for (auto childIter : m_children)
            {
                if (childIter.m_execution)
                {
                    childIter.m_execution->Clear();
                }
            }

            m_children.clear();
            m_input.clear();
            m_inputConversion.clear();
            ClearProperyExtractionSources();

            for (auto returnValue : m_returnValues)
            {
                if (auto returnValuePtr = AZStd::const_pointer_cast<ReturnValue>(returnValue.second))
                {
                    returnValuePtr->Clear();
                }
            }
            m_returnValues.clear();

            m_scope = nullptr;
        }

        void ExecutionTree::ClearChildren()
        {
            m_children.clear();
        }

        void ExecutionTree::ClearInput()
        {
            m_input.clear();
        }

        void ExecutionTree::ClearProperyExtractionSources()
        {
            m_propertyExtractionsSource.clear();
        }

        void ExecutionTree::ConvertNameToIdentifier()
        {
            m_name = ToIdentifierSafe(m_name);
        }

        void ExecutionTree::CopyInput(ExecutionTreeConstPtr source, RemapVariableSource remapSource)
        {
            m_input = source->m_input;

            if (remapSource == RemapVariableSource::Yes)
            {
                for (auto& input : m_input)
                {
                    if (input.m_value->m_source == source)
                    {
                        input.m_value->m_source = shared_from_this();
                    }
                }
            }
        }

        void ExecutionTree::CopyReturnValuesToInputs(ExecutionTreeConstPtr source)
        {
            AZ_Assert(m_input.empty(), "mixing return values with input");

            if (!m_in.m_node)
            {
                return;
            }

            for (auto& returnValue : source->m_returnValues)
            {
                // only copy the value if the out call has the slot in question
                if (auto slot = m_in.m_node->GetSlot(returnValue.second->m_source->m_sourceSlotId))
                {
                    m_input.push_back({ nullptr, returnValue.second->m_source, DebugDataSource::FromSelfSlot(*slot) });
                }
            }
        }

        size_t ExecutionTree::FindIndexOfChild(ExecutionTreeConstPtr child) const
        {
            auto iter = AZStd::find_if
                ( m_children.begin()
                , m_children.end()
                , [&](auto& candidate)->bool { return candidate.m_execution == child; });

            return iter - m_children.begin();
        }

        ExecutionChild* ExecutionTree::FindChild(const SlotId slotId)
        {
            return const_cast<ExecutionChild*>(FindChildConst(slotId));
        }

        const ExecutionChild* ExecutionTree::FindChildConst(ExecutionTreeConstPtr execution) const
        {
            auto child = AZStd::find_if
                ( m_children.begin()
                , m_children.end()
                , [&](const auto& candidate) { return candidate.m_execution == execution; });

            return child != m_children.end() ? child : nullptr;
        }

        const ExecutionChild* ExecutionTree::FindChildConst(const SlotId slotId) const
        {
            auto child = AZStd::find_if
                ( m_children.begin()
                , m_children.end()
                , [&](const auto& candidate) { return candidate.m_slot && candidate.m_slot->GetId() == slotId; });

            return child != m_children.end() ? child : nullptr;
        }

        size_t ExecutionTree::FindChildIndex(ExecutionTreeConstPtr execution) const
        {
            auto child = FindChildConst(execution);
            return child ? child - m_children.begin() : std::numeric_limits<size_t>::max();
        }

        const ExecutionChild& ExecutionTree::GetChild(size_t index) const
        {
            return m_children[index];
        }

        size_t ExecutionTree::GetChildrenCount() const
        {
            return m_children.size();
        }

        const ConversionByIndex& ExecutionTree::GetConversions() const
        {
            return m_inputConversion;
        }

        PropertyExtractionConstPtr ExecutionTree::GetExecutedPropertyExtraction() const
        {
            return m_propertyExtractionExecuted;
        }

        EventType ExecutionTree::GetEventType() const
        {
            return m_eventType;
        }

        const ExecutionId& ExecutionTree::GetId() const
        {
            return m_in;
        }

        AZ::EntityId ExecutionTree::GetNodeId() const
        {
            return m_in.m_node ? m_in.m_node->GetEntityId() : AZ::EntityId();
        }

        const ExecutionInput& ExecutionTree::GetInput(size_t index) const
        {
            return m_input[index];
        }

        size_t ExecutionTree::GetInputCount() const
        {
            return m_input.size();
        }

        AZStd::vector<ExecutionTreeConstPtr> ExecutionTree::GetInternalOuts() const
        {
            AZStd::vector<ExecutionTreeConstPtr> outs;

            for (auto childIter : m_children)
            {
                if (childIter.m_execution && childIter.m_execution->IsInternalOut())
                {
                    outs.push_back(childIter.m_execution);
                }
            }

            return outs;
        }

        const AZStd::vector<AZStd::pair<const Slot*, OutputAssignmentConstPtr>>* ExecutionTree::GetLocalOutput() const
        {
            return m_children.size() == 1 ? &m_children[0].m_output : nullptr;
        }

        MetaDataConstPtr ExecutionTree::GetMetaData() const
        {
            return m_metaData;
        }

        const AZStd::string& ExecutionTree::GetName() const
        {
            return m_name;
        }
        
        const LexicalScope& ExecutionTree::GetNameLexicalScope() const
        {
            return m_lexicalScope;
        }

        VariableConstPtr ExecutionTree::GetNodeable() const
        {
            return m_nodeable;
        }

        AZStd::optional<size_t> ExecutionTree::GetOutCallIndex() const
        {
            return m_outCallIndex != std::numeric_limits<size_t>::max() ? AZStd::optional<size_t>(m_outCallIndex) : AZStd::nullopt;
        }

        ExecutionTreeConstPtr ExecutionTree::GetParent() const
        {
            return m_parent;
        }

        ExecutionTreeConstPtr ExecutionTree::GetRoot() const
        {
            return const_cast<ExecutionTree*>(this)->ModRoot();
        }

        const AZStd::vector<AZStd::pair<const Slot*, PropertyExtractionConstPtr>>& ExecutionTree::GetPropertyExtractionSources() const
        {
            return m_propertyExtractionsSource;
        }

        AZStd::pair<const Slot*, ReturnValueConstPtr> ExecutionTree::GetReturnValue(size_t index) const
        {
            return m_returnValues[index];
        }

        size_t ExecutionTree::GetReturnValueCount() const
        {
            return m_returnValues.size();
        }

        ScopeConstPtr ExecutionTree::GetScope() const
        {
            return m_scope;
        }

        Symbol ExecutionTree::GetSymbol() const
        {
            return m_symbol;
        }

        bool ExecutionTree::HasExplicitUserOutCalls() const
        {
            return m_hasExplicitUserOutCalls;
        }

        bool ExecutionTree::HasReturnValues() const
        {
            return !m_returnValues.empty();
        }

        bool ExecutionTree::InputHasThisPointer() const
        {
            return m_inputHasThisPointer;
        }

        bool ExecutionTree::IsInfiniteLoopDetectionPoint() const
        {
            return m_isInfiniteLoopDetectionPoint;
        }

        void ExecutionTree::InsertChild(size_t index, const ExecutionChild& child)
        {
            m_children.insert(m_children.begin() + index, child);
        }

        bool ExecutionTree::IsInputOutputPreprocessed() const
        {
            return m_isInputOutputPreprocessed;
        }

        bool ExecutionTree::IsInternalOut() const
        {
            return m_isInternalOut;
        }

        bool ExecutionTree::IsOnLatentPath() const
        {
            if (m_isLatent)
            {
                return true;
            }

            auto parent = GetParent();
            if (!parent)
            {
                return false;
            }

            return parent->IsOnLatentPath();
        }

        bool ExecutionTree::IsPure() const
        {
            return GetRoot()->m_isPure;
        }

        bool ExecutionTree::IsStartCall() const
        {
            return m_isStartCall;
        }

        void ExecutionTree::MarkDebugEmptyStatement()
        {
            if (GetSymbol() != Symbol::UserOut && (GetChildrenCount() == 0 || GetSymbol() == Symbol::PlaceHolderDuringParsing))
            {
                SetSymbol(Symbol::DebugInfoEmptyStatement);
            }
        }

        void ExecutionTree::MarkHasExplicitUserOutCalls()
        {
            m_hasExplicitUserOutCalls = true;
        }

        void ExecutionTree::MarkInfiniteLoopDetectionPoint()
        {
            m_isInfiniteLoopDetectionPoint = true;
        }

        void ExecutionTree::MarkInputHasThisPointer()
        {
            m_inputHasThisPointer = true;
        }

        void ExecutionTree::MarkInputOutputPreprocessed()
        {
            m_isInputOutputPreprocessed = true;
        }

        void ExecutionTree::MarkInternalOut()
        {
            m_isInternalOut = true;
        }

        void ExecutionTree::MarkPure()
        {
            m_isPure = true;    
        }

        void ExecutionTree::MarkRefersToSelfEntityId()
        {
            ModRoot()->m_refersToSelfEntityId = true;
        }

        void ExecutionTree::MarkRootLatent()
        {
            auto root = ModRoot();
            root->m_isLatent = true;
        }

        void ExecutionTree::MarkStartCall()
        {
            m_isStartCall = true;
        }

        ExecutionChild& ExecutionTree::ModChild(size_t index)
        {
            return m_children[index];
        }

        ConversionByIndex& ExecutionTree::ModConversions()
        {
            return m_inputConversion;
        }

        ExecutionInput& ExecutionTree::ModInput(size_t index)
        {
            return m_input[index];
        }

        MetaDataPtr ExecutionTree::ModMetaData()
        {
            return m_metaData;
        }

        ExecutionTreePtr ExecutionTree::ModParent()
        {
            return m_parent;
        }

        ExecutionTreePtr ExecutionTree::ModRoot()
        {
            if (m_symbol == Symbol::FunctionDefinition)
            {
                return shared_from_this();
            }          

            auto parent = ModParent();
            if (!parent)
            {
                return nullptr;
            }

            return parent->ModRoot();
        }

        ScopePtr ExecutionTree::ModScope()
        {
            return m_scope;
        }

        ScopePtr ExecutionTree::ModScopeFunction()
        {
            if (m_symbol == Symbol::FunctionDefinition)
            {
                return m_scope;
            }

            auto parent = ModParent();
            if (!parent)
            {
                return nullptr;
            }

            return parent->ModScopeFunction();
        }

        void ExecutionTree::ReduceInputSet(const AZ::InputRestriction& restriction)
        {
            if (restriction.m_listExcludes)
            {
                if (!restriction.m_indices.empty())
                {
                    // exclusive culling
                    AZ::InputRestriction restrictionInverse;
                    restrictionInverse.m_listExcludes = false;

                    auto begin = restriction.m_indices.begin();
                    auto end = restriction.m_indices.end();

                    for (size_t index = 0; index < m_input.size(); ++index)
                    {
                        if (AZStd::find(begin, end, index) == end)
                        {
                            restrictionInverse.m_indices.push_back(aznumeric_caster(index));
                        }
                    }

                    ReduceInputSet(restrictionInverse);
                }
            }
            else if (restriction.m_indices.empty())
            {
                // inclusive culling
                m_input.clear();
            }
            else
            {
                // inclusive culling
                AZStd::vector<ExecutionInput> newInput;
                ConversionByIndex newInputConversion;

                for (auto index : restriction.m_indices)
                {
                    newInput.push_back(m_input[index]);

                    auto inputConversionIter = m_inputConversion.find(index);
                    if (inputConversionIter != m_inputConversion.end())
                    {
                        newInputConversion.insert({ newInput.size(), inputConversionIter->second });
                    }
                }

                m_input = newInput;
                m_inputConversion = newInputConversion;
            }
        }

        bool ExecutionTree::RefersToSelfEntityId() const
        {
            return GetRoot()->m_refersToSelfEntityId;
        }

        AZ::Outcome<AZStd::pair<size_t, ExecutionChild>> ExecutionTree::RemoveChild(const ExecutionTreeConstPtr& child)
        {
            auto iter = AZStd::find_if(m_children.begin(), m_children.end(), [&](const auto& candidate) { return candidate.m_execution == child; });
            if (iter != m_children.end())
            {
                auto returnValue = *iter;
                const size_t index = iter - m_children.begin();
                m_children.erase(iter);
                return AZ::Success(AZStd::make_pair(index, returnValue));
            }
            else
            {
                return AZ::Failure();
            }
        }

        void ExecutionTree::SetExecutedPropertyExtraction(PropertyExtractionConstPtr propertyExtraction)
        {
            m_propertyExtractionExecuted = propertyExtraction;
        }

        void ExecutionTree::SetEventType(EventType eventType)
        {
            m_eventType = eventType;
        }

        void ExecutionTree::SetId(const ExecutionId& id)
        {
            m_in = id;
        }

        void ExecutionTree::SetMetaData(MetaDataPtr metaData)
        {
            m_metaData = metaData;
        }

        void ExecutionTree::SetNodeable(VariableConstPtr nodeable)
        {
            m_nodeable = nodeable;
        }

        void ExecutionTree::SetName(AZStd::string_view name)
        {
            m_name = name;
        }

        void ExecutionTree::SetNameLexicalScope(const LexicalScope& lexicalScope)
        {
            m_lexicalScope = lexicalScope;
        }

        void ExecutionTree::SetOutCallIndex(size_t index)
        {
            m_outCallIndex = index;
        }

        void ExecutionTree::SetParent(ExecutionTreePtr parent)
        {
            m_parent = parent;
        }

        void ExecutionTree::SetScope(ScopePtr scope)
        {
            m_scope = scope;
        }

        void ExecutionTree::SetSymbol(Symbol val)
        {
            m_symbol = val;
        }

        void ExecutionTree::SwapChildren(ExecutionTreePtr execution)
        {
            if (execution)
            {
                m_children.swap(execution->m_children);

                for (auto& child : m_children)
                {
                    if (child.m_execution)
                    {
                        child.m_execution->SetParent(shared_from_this());
                    }
                }

                for (auto& orphan : execution->m_children)
                {
                    if (orphan.m_execution)
                    {
                        orphan.m_execution->SetParent(execution);
                    }
                }
            }
            else
            {
                ClearChildren();
            }
        }
    }
} 
