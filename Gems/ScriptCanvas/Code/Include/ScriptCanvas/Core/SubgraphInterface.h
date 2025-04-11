/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/any.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Outcome/Outcome.h>

#include "ScriptCanvas/Data/Data.h"
#include "ScriptCanvas/Core/Datum.h"
#include "ScriptCanvas/Variable/VariableCore.h"
#include "ScriptCanvas/Grammar/ParsingUtilities.h"

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    namespace Grammar
    {
        using FunctionSourceId = AZ::Uuid;

        template<typename T>
        void SetDisplayAndParsedNameSafe(T& t, const AZStd::string& name)
        {
            t.displayName = name;
            t.parsedName = ToIdentifierSafe(name);
        }

        template<typename T>
        void SetDisplayAndParsedName(T& t, const AZStd::string& name)
        {
            t.displayName = name;
            t.parsedName = ToIdentifier(name);
        }

        struct Input final
        {
            AZ_TYPE_INFO(Input, "{627448C3-D018-422E-B133-A1169BB44306}");
            AZ_CLASS_ALLOCATOR(Input, AZ::SystemAllocator); 
        
            AZStd::string displayName;
            AZStd::string parsedName;
            Datum datum;
            VariableId sourceID;

            bool operator==(const Input& rhs) const;
        };
        using Inputs = AZStd::vector<Input>;

        struct Output final
        {
            AZ_TYPE_INFO(Output, "{344D66C7-EE5E-45B1-809F-4108DDB65F20}");
            AZ_CLASS_ALLOCATOR(Output, AZ::SystemAllocator);

            AZStd::string displayName;
            AZStd::string parsedName;
            Data::Type type;
            VariableId sourceID;

            bool operator==(const Output& rhs) const;
        };
        using Outputs = AZStd::vector<Output>;

        struct Out final
        {
            AZ_TYPE_INFO(Out, "{6175D897-C06D-48B5-8775-388B232D429D}");
            AZ_CLASS_ALLOCATOR(Out, AZ::SystemAllocator);

            AZStd::string displayName;
            AZStd::string parsedName;
            Outputs outputs;
            Inputs returnValues;
            FunctionSourceId sourceID;

            bool operator==(const Out& rhs) const;
        };
        using Outs = AZStd::vector<Out>;

        struct In final
        {
            AZ_TYPE_INFO(In, "{DFDA32F7-41D2-45BB-8ADF-876679053836}");
            AZ_CLASS_ALLOCATOR(In, AZ::SystemAllocator); 
            
            bool isPure = false;
            AZStd::string displayName;
            AZStd::string parsedName;
            Inputs inputs;
            Outs outs;
            FunctionSourceId sourceID;
            
            bool operator==(const In& rhs) const;
            // #functions2 In-sensitive parsing. convert pure branches to mini-nodeables and provide a creation function
            inline bool IsBranch() const { return outs.size() > 1; }
        };
        using Ins = AZStd::vector<In>;

        class SubgraphInterface final
        {
        public:
            AZ_TYPE_INFO(SubgraphInterface, "{52B27A11-8294-4A6F-BFCF-6C1582649DB2}");
            AZ_CLASS_ALLOCATOR(SubgraphInterface, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* refectContext);

            SubgraphInterface() = default;

            SubgraphInterface(Ins&& ins);
            
            SubgraphInterface(Ins&& ins, Outs&& latents);

            SubgraphInterface(Outs&& latents);

            void AddIn(In&& in);

            void AddLatent(Out&& out);

            const In* FindIn(const FunctionSourceId& sourceID) const;

            const Out* FindLatent(const FunctionSourceId& sourceID) const;

            ExecutionCharacteristics GetExecutionCharacteristics() const;

            const In* GetIn(const AZStd::string& in) const;

            const In& GetIn(size_t index) const;

            const Ins& GetIns() const;

            size_t GetInCount() const;

            size_t GetInCountPure() const;

            size_t GetInCountNotPure() const;

            const Inputs* GetInput(const AZStd::string& in) const;

            const Out& GetLatentOut(size_t index) const;

            size_t GetLatentOutCount() const;

            const Outputs* GetLatentOutput(const AZStd::string& latent) const;

            const Outs& GetLatentOuts() const;

            LexicalScope GetLexicalScope() const;

            LexicalScope GetLexicalScope(const In& in) const;

            AZ::Outcome<AZStd::string, AZStd::string> GetName() const;

            const NamespacePath& GetNamespacePath() const;

            const Out* GetOut(const AZStd::string& in, const AZStd::string& out) const;
            
            // used to initialize out map for "free"
            const AZStd::vector<AZ::Crc32>& GetOutKeys() const;
                        
            const Outputs* GetOutput(const AZStd::string& in, const AZStd::string& out) const;

            const Outs* GetOuts(const AZStd::string& in) const;           

            // meaningless (and empty) if not an object
            const AZStd::string& GetParentClassName() const;

            bool IsActiveDefaultObject() const;

            bool IsBaseClass() const;

            AZ::Outcome<bool> IsBranch(const AZStd::string& in) const;

            bool IsClass() const;

            // returns true iff there is at least one latent out
            bool IsLatent() const;

            bool IsMarkedPure() const;

            bool IsParsedPure() const;

            bool IsUserNodeable() const;

            bool HasAnyFunctionality() const;

            bool HasBranches() const;

            bool HasIn(const FunctionSourceId& sourceID) const;

            bool HasInput(const VariableId& sourceID) const;

            bool HasLatent(const FunctionSourceId& sourceID) const;

            bool HasOnGraphStart() const;

            bool HasOut(const FunctionSourceId& sourceID) const;

            bool HasOutput(const VariableId& sourceID) const;

            // returns true iff there is no public access to Ins or Latents
            // \note The subgraph could still DO something, and be useful, it could respond to tick bus or on graph start
            bool HasPublicFunctionality() const;

            void MarkActiveDefaultObject();          

            void MarkExecutionCharacteristics(ExecutionCharacteristics characteristics);

            void MarkOnGraphStart();

            void MarkRefersToSelfEntityId();

            void MarkRequiresConstructionParameters();

            void MarkRequiresConstructionParametersForDependencies();

            // #sc_user_data_type expose this to users directly, so they can say "I don't just want to make a component with this, I want to drop it right into another graph" 
            void MarkClass();

            void MergeExecutionCharacteristics(const SubgraphInterface & dependency);

            In* ModIn(const FunctionSourceId & sourceID);

            bool operator==(const SubgraphInterface& rhs) const;

            // Populates the list of out keys
            AZ::Outcome<void, AZStd::string> Parse();

            bool RefersToSelfEntityId() const;

            bool RequiresConstructionParameters() const;

            bool RequiresConstructionParametersForDependencies() const;

            void MarkBaseClass();

            void SetNamespacePath(const NamespacePath& namespacePath);

            AZStd::string ToExecutionString() const;

        private:
            bool m_isClass = false;

            bool m_areAllChildrenPure = true;

            // Does this graph have any (automatic) connection to buses or other latent activity, or even on graph start
            // regardless of public exposure to in/out?
            bool m_isActiveDefaultObject = false;

            bool m_hasOnGraphStart = false;

            bool m_requiresConstructionParameters = false;

            bool m_requiresConstructionParametersForDependencies = false;

            // #scriptcanvas_component_extension
            bool m_refersToSelfEntityId = false;

            ExecutionCharacteristics m_executionCharacteristics = ExecutionCharacteristics::Pure;

            Ins m_ins;
            Outs m_latents;
            AZStd::vector<AZ::Crc32> m_outKeys;
            NamespacePath m_namespacePath;
            bool m_isBaseClass = true;
            AZStd::string m_parentClassName;

            bool AddOutKey(const AZStd::string& name);
            const Out* FindImmediateOut(const AZStd::string& in, const AZStd::string& out) const;
            const In* FindIn(const AZStd::string& inSlotId) const;
            const Out* FindLatentOut(const AZStd::string& latent) const;
            LexicalScope GetLexicalScope(bool isSourcePure) const;
        };

        AZStd::string ToString(const In& in, size_t tabs = 0);

        AZStd::string ToString(const Ins& ins, size_t tabs = 0);

        AZStd::string ToString(const Input& input, size_t tabs = 1);

        AZStd::string ToString(const Inputs& inputs, size_t tabs = 1);

        AZStd::string ToString(const Out& out, bool isLatent, size_t tabs = 0);

        AZStd::string ToString(const Outs& out, bool isLatent, size_t tabs = 0);

        AZStd::string ToString(const Output& output, size_t tabs = 1);

        AZStd::string ToString(const Outputs& outputs, size_t tabs = 1);

        AZStd::string ToString(const SubgraphInterface& subgraphInterface);
        
        using SubgraphInterfacePtrConst = AZStd::shared_ptr<const SubgraphInterface>;
        using InterfacesByNodeType = AZStd::unordered_map<FunctionSourceId, SubgraphInterfacePtrConst>;

        class SubgraphInterfaceSystem
        {
        public:
            AZ_CLASS_ALLOCATOR(SubgraphInterfaceSystem, AZ::SystemAllocator);

            // returns an execution map for the node if it has registered one.
            // if it hasn't, the node must be considered simple, as has no need for one
            SubgraphInterfacePtrConst GetMap(const FunctionSourceId& nodeTypeId) const;
           
            // if this is true, it means all input data is required for all input executions slots,
            // and there is only one, immediate, execution out slots (which will use all data)
            // and no latent execution out slots
            // 
            // @note Call this method first, if it returns true, every other query about a node will error.
            bool IsSimple(const FunctionSourceId& nodeTypeId) const;

            bool RegisterMap(const FunctionSourceId& nodeTypeId, SubgraphInterfacePtrConst executionMap);

        private:
            InterfacesByNodeType m_mapsByNodeType;
        };
    }
} 
