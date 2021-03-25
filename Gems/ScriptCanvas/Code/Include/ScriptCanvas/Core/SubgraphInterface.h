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
            AZ_CLASS_ALLOCATOR(Input, AZ::SystemAllocator, 0); 
        
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
            AZ_CLASS_ALLOCATOR(Output, AZ::SystemAllocator, 0);

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
            AZ_CLASS_ALLOCATOR(Out, AZ::SystemAllocator, 0);

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
            AZ_CLASS_ALLOCATOR(In, AZ::SystemAllocator, 0); 
            
            AZStd::string displayName;
            AZStd::string parsedName;
            Inputs inputs;
            Outs outs;
            FunctionSourceId sourceID;

            bool operator==(const In& rhs) const;

            AZ_INLINE bool IsBranch() const { return outs.size() > 1; }
        };
        using Ins = AZStd::vector<In>;

        class SubgraphInterface final
        {
        public:
            AZ_TYPE_INFO(SubgraphInterface, "{52B27A11-8294-4A6F-BFCF-6C1582649DB2}");
            AZ_CLASS_ALLOCATOR(SubgraphInterface, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* refectContext);

            SubgraphInterface() = default;

            SubgraphInterface(Ins&& ins);
            
            SubgraphInterface(Ins&& ins, Outs&& latents);

            SubgraphInterface(Outs&& latents);

            void AddIn(In&& in);

            void AddLatent(Out&& out);

            ExecutionCharacteristics GetExecutionCharacteristics() const;

            const In* GetIn(const AZStd::string& in) const;

            const In& GetIn(size_t index) const;

            const Ins& GetIns() const;

            size_t GetInCount() const;

            const Inputs* GetInput(const AZStd::string& in) const;
            
            const Out& GetLatentOut(size_t index) const;

            size_t GetLatentOutCount() const;

            const Outputs* GetLatentOutput(const AZStd::string& latent) const;

            const Outs& GetLatentOuts() const;

            // \todo for max optimization this would be In sensitive
            LexicalScope GetLexicalScope() const;

            AZStd::string GetName() const;

            const NamespacePath& GetNamespacePath() const;

            const Out* GetOut(const AZStd::string& in, const AZStd::string& out) const;
            
            // used to initialize out map for "free"
            const AZStd::vector<AZ::Crc32>& GetOutKeys() const;
                        
            const Outputs* GetOutput(const AZStd::string& in, const AZStd::string& out) const;

            const Outs* GetOuts(const AZStd::string& in) const;           

            bool IsActiveDefaultObject() const;

            bool IsAllInputOutputShared() const;

            AZ::Outcome<bool> IsBranch(const AZStd::string& in) const;

            // returns true iff there is at least one latent out
            bool IsLatent() const;

            bool IsMarkedPure() const;

            bool IsParsedPure() const;

            bool HasBranches() const;

            bool HasIn(const FunctionSourceId& sourceID) const;

            bool HasInput(const VariableId& sourceID) const;

            bool HasLatent(const FunctionSourceId& sourceID) const;

            bool HasOnGraphStart() const;

            bool HasOut(const FunctionSourceId& sourceID) const;

            bool HasOutput(const VariableId& sourceID) const;

            // returns true iff there is no public access to Ins or Latents
            // \note The subgraph could still DO something, and be useful
            bool HasPublicFunctionality() const;

            void MarkActiveDefaultObject();

            void MarkAllInputOutputShared();

            void MarkExecutionCharacteristics(ExecutionCharacteristics characteristics);

            void MarkOnGraphStart();

            void MergeExecutionCharacteristics(const SubgraphInterface& dependency);

            bool operator==(const SubgraphInterface& rhs) const;

            // Populates the list of out keys
            void Parse();

            void SetNamespacePath(const NamespacePath& namespacePath);

            void TakeNamespacePath(NamespacePath&& namespacePath);

            AZStd::string ToExecutionString() const;

        private:
            bool m_areAllChildrenPure = true;

            // Does this graph have any (automatic) connection to buses or other latent activity, 
            // regardless of public exposure to out?
            bool m_isActiveDefaultObject = false;

            // all input/output are used in every in/out/latent slot?
            bool m_isAllInputOutputShared = false;

            bool m_hasOnGraphStart = false;

            ExecutionCharacteristics m_executionCharacteristics = ExecutionCharacteristics::Pure;

            Ins m_ins;
            Outs m_latents;
            AZStd::vector<AZ::Crc32> m_outKeys;
            NamespacePath m_namespacePath;

            void AddOutKey(const AZStd::string& name);
            const Out* FindImmediateOut(const AZStd::string& in, const AZStd::string& out) const;
            const In* FindIn(const AZStd::string& inSlotId) const;
            const Out* FindLatentOut(const AZStd::string& latent) const;
        };

        using SubgraphInterfacePtrConst = AZStd::shared_ptr<const SubgraphInterface>;
        using InterfacesByNodeType = AZStd::unordered_map<FunctionSourceId, SubgraphInterfacePtrConst>;

        class SubgraphInterfaceSystem
        {
        public:
            AZ_CLASS_ALLOCATOR(SubgraphInterfaceSystem, AZ::SystemAllocator, 0);

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
