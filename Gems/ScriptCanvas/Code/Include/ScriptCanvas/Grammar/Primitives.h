
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Variable/VariableCore.h>

#include "DebugMap.h"
#include "PrimitivesDeclarations.h"

namespace ScriptCanvas
{
    class Nodeable;
    namespace Grammar
    {
        enum class Direction
        {
            In,
            Out
        };

        enum class EventHandingType
        {
            EBus,
            Event,
            VariableWrite,
            Count
        };

        enum class LexicalScopeType
        {
            Class,
            Namespace,
            Self,
            Variable,
        };

        enum class NodelingType
        {
            In,
            None,
            Out,
            OutReturn,
        };

        enum TraitsFlags
        {
            Const = 1 << 0,
            Member = 1 << 1,
            Public = 1 << 2,
            Static = 1 << 3,
        };
        
        const char* GetSymbolName(Symbol nodeType);

        struct EBusBase
        {
            AZ_TYPE_INFO(EBusBase, "{A29AF0FF-5E2E-404C-AA8A-029AEC67FB1F}");
            AZ_CLASS_ALLOCATOR(EBusBase, AZ::SystemAllocator);

            bool m_isEverConnected = false;
            bool m_isEverDisconnected = false;
            bool m_startsConnected = false;
            bool m_isAutoConnected = false;

            // \todo this could be mildly improved to be order sensitive, since that will be known
            // but will be a low optimization priority
            bool RequiresConnectionControl() const;
        };

        struct EBusHandling
            : public AZStd::enable_shared_from_this<EBusHandling>
            , public EBusBase
        {
            AZ_TYPE_INFO(EBusHandling, "{CD45249C-3CC8-4AAD-B61E-8CCDC05144B7}");
            AZ_CLASS_ALLOCATOR(EBusHandling, AZ::SystemAllocator);

            bool m_isAddressed = false;
            const Node* m_node = nullptr;
            VariableConstPtr m_startingAdress;
            AZStd::string m_ebusName;
            AZStd::string m_handlerName;
            AZStd::vector<AZStd::pair<AZStd::string, ExecutionTreeConstPtr>> m_events;
            void Clear();
        };

        struct EventHandling
            : public AZStd::enable_shared_from_this<EventHandling>
        {
            AZ_TYPE_INFO(EventHandling, "{D4E21276-141D-440D-A529-BCC691A9E906}");

            const Node* m_eventNode = nullptr;
            const Slot* m_eventSlot = nullptr;
            AZStd::string m_eventName;
            AZStd::string m_handlerName;
            VariableConstPtr m_handler;
            ExecutionTreeConstPtr m_eventHandlerFunction;

            void Clear();
        };

        struct FunctionPrototype
        {
            AZ_TYPE_INFO(FunctionPrototype, "{7785B43E-102A-4E66-87F6-E59D37C4DBB2}");
            AZ_CLASS_ALLOCATOR(FunctionPrototype, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* reflectContext);

            // parameters (Variables provide more info than datum, but they are not live variables)
            AZStd::vector<VariableConstPtr> m_inputs;

            // (possible) return value (Variables provide more info than datum, but they are not live variables)
            AZStd::vector<VariableConstPtr> m_outputs;

            void Clear();

            // returns true if no return value
            bool IsVoid() const;

            bool operator==(const FunctionPrototype& other) const;
        };

        struct LexicalScope
        {
            AZ_TYPE_INFO(LexicalScope, "{98162B8F-BA67-4476-89E7-53F5569836B9}");
            AZ_CLASS_ALLOCATOR(LexicalScope, AZ::SystemAllocator);

            LexicalScopeType m_type = LexicalScopeType::Namespace;
            AZStd::vector<AZStd::string> m_namespaces;

            LexicalScope() = default;

            LexicalScope(LexicalScopeType type);

            LexicalScope(LexicalScopeType type, const AZStd::vector<AZStd::string>& namespaces);

            static LexicalScope Global();

            static LexicalScope Variable();
        };

        struct MetaData
            : public AZStd::enable_shared_from_this<MetaData>
        {
            AZ_RTTI(MetaData, "{1C663A26-F405-481D-BCC6-1F16A7A5DE9E}");
            AZ_CLASS_ALLOCATOR(MetaData, AZ::SystemAllocator);

            virtual ~MetaData() = default;

            virtual void PostParseExecutionTreeBody(AbstractCodeModel& /*model*/, ExecutionTreePtr /*execution*/) {}
        };

        // for now, no return values supported
        struct MultipleFunctionCallFromSingleSlotEntry
        {
            AZ_TYPE_INFO(MultipleFunctionCallFromSingleSlotEntry, "{360A23A3-C490-4047-B71E-64E290E441D3}");
            AZ_CLASS_ALLOCATOR(MultipleFunctionCallFromSingleSlotEntry, AZ::SystemAllocator);

            bool isVariadic = false;
            AZStd::string functionName;
            LexicalScope lexicalScope;
            size_t numArguments = 0; // stride in case isVariadic == true
            size_t startingIndex = 0; // the index of the slot order
        };

        // for now, no return values supported
        struct MultipleFunctionCallFromSingleSlotInfo
        {
            AZ_TYPE_INFO(MultipleFunctionCallFromSingleSlotInfo, "{DF51F08A-8B28-4851-9888-9AB7CC0B90D2}");
            AZ_CLASS_ALLOCATOR(MultipleFunctionCallFromSingleSlotInfo, AZ::SystemAllocator);

            // this could likely be implemented, but needs care to duplicate input that the execution-slot created
            // bool errorOnReusedSlot = false;

            bool errorOnUnusedSlot = false;

            // calls are executed in the order they arrive in the vector
            AZStd::vector<MultipleFunctionCallFromSingleSlotEntry> functionCalls;
        };

        struct NodeableParse
            : public AZStd::enable_shared_from_this<NodeableParse>
        {
            AZ_RTTI(NodeableParse, "{72D8C7AA-E860-4806-B6AC-4A57EAD9AD22}");
            AZ_CLASS_ALLOCATOR(NodeableParse, AZ::SystemAllocator);

            virtual ~NodeableParse() {}

            VariableConstPtr m_nodeable;
            bool m_isInterpreted = false;
            AZStd::string m_simpleName;
            AZStd::vector<ExecutionTreeConstPtr> m_onInputChanges;
            AZStd::vector<AZStd::pair<AZStd::string, ExecutionTreeConstPtr>> m_latents;

            void Clear();
        };

        struct ParsedRuntimeInputs
        {
            AZStd::vector<Nodeable*> m_nodeables;
            AZStd::vector<AZStd::pair<VariableId, Datum>> m_variables;
            // either the entityId was a (member) variable in the source graph, or it got promoted to one during parsing
            AZStd::vector<AZStd::pair<VariableId, Data::EntityIDType>> m_entityIds;
            // Statics required for internal, local values that need non-code constructible initialization,
            // when the system can't pass in the input from C++.
            AZStd::vector<AZStd::pair<VariableId, AZStd::any>> m_staticVariables;
            bool m_refersToSelfEntityId;
        };

        struct PropertyExtraction
            : public AZStd::enable_shared_from_this<PropertyExtraction>
        {
            AZ_TYPE_INFO(PropertyExtraction, "{ACA69D23-5132-4E3E-A17F-01E354BA3B6B}");
            AZ_CLASS_ALLOCATOR(PropertyExtraction, AZ::SystemAllocator);

            const Slot* m_slot = nullptr;
            AZStd::string m_name;
        };

        struct OutputAssignment
            : public AZStd::enable_shared_from_this<OutputAssignment>
        {
            AZ_TYPE_INFO(OutputAssignment, "{8A6281F4-403A-4A63-919B-633A4BF83901}");
            AZ_CLASS_ALLOCATOR(OutputAssignment, AZ::SystemAllocator);
            
            VariableConstPtr m_source; // the actual result of the function

            AZStd::vector<VariableConstPtr> m_assignments; // by reference or return value assignments
            ConversionByIndex m_sourceConversions;

            void Clear();
        };
        
        struct ReturnValue
            : public OutputAssignment
        {
            AZ_TYPE_INFO(ReturnValue, "{2B7F0129-91F7-4662-8D31-E8DE72975ECC}");
            AZ_CLASS_ALLOCATOR(ReturnValue, AZ::SystemAllocator)

            VariableConstPtr m_initializationValue;
            bool m_isNewValue = true;
            ReturnValue(OutputAssignment&& source);
            DebugDataSource m_sourceDebug;
            void Clear();
        };

        struct Scope
            : public AZStd::enable_shared_from_this<Scope>
        {
            AZ_TYPE_INFO(Scope, "{E7FF5F8A-B98B-4609-B1DA-7A7F9729A34F}");
            AZ_CLASS_ALLOCATOR(Scope, AZ::SystemAllocator);

            ScopeConstPtr m_parent;

            AZStd::string AddFunctionName(AZStd::string_view name);

            AZStd::string AddVariableName(AZStd::string_view name);

            AZStd::string AddVariableName(AZStd::string_view name, AZStd::string_view suffix);

        private:
            AZStd::unordered_map<AZStd::string, AZ::s32> m_baseNameToCount;

            AZ::s32 AddNameCount(AZStd::string_view name);
        };

        struct Variable
            : public AZStd::enable_shared_from_this<Variable>
        {
            AZ_TYPE_INFO(Variable, "{B249512C-A4D2-4EA0-9F86-409A0C22CC57}");
            AZ_CLASS_ALLOCATOR(Variable, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* reflectContext);

            ExecutionTreeConstPtr m_source; // the execution that produced this variable
            SlotId m_sourceSlotId; // to broadcast changes in debug view, needed for handled event arguments
            VariableId m_sourceVariableId; // to broadcast changes in debug view
            AZ::EntityId m_nodeableNodeId; // to broadcast changes in debug view
            Datum m_datum;
            AZStd::string m_name;
            bool m_isConst = false;
            bool m_isMember = false;
            bool m_requiresNullCheck = false;
            bool m_initializeAsNull = false;
            bool m_requiresCreationFunction = false;
            bool m_isUnused = false; // used for multiple return situations, and to prevent compile errors
            bool m_isExposedToConstruction = false;
            bool m_isDebugOnly = false;
            bool m_isFromFunctionDefinitionSlot = false;

            Variable() = default;
            Variable(Datum&& datum);
            Variable(const Datum& datum, const AZStd::string& name, TraitsFlags traitsFlags);
            Variable(Datum&& datum, AZStd::string&& name, TraitsFlags&& traitsFlags);
        };

        struct VariableWriteHandling
            : public AZStd::enable_shared_from_this<VariableWriteHandling>
            , public EBusBase
        {
            AZ_TYPE_INFO(VariableWriteHandling, "{C60BD93A-B44F-4345-A9EA-4200DD97CFA6}");
            AZ_CLASS_ALLOCATOR(VariableWriteHandling, AZ::SystemAllocator);
            
            VariableConstPtr m_variable;
            VariableConstPtr m_connectionVariable;
            ExecutionTreeConstPtr m_function;

            void Clear();
        };

    } 

} 

