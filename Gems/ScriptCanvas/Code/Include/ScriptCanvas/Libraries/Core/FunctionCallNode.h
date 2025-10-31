/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>
#include <ScriptCanvas/Asset/SubgraphInterfaceAsset.h>
#include <ScriptCanvas/Core/SubgraphInterface.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/SlotExecutionMap.h>
#include <ScriptCanvas/Libraries/Core/FunctionDefinitionNode.h>
#include <ScriptCanvas/Utils/VersioningUtils.h>

#include <Include/ScriptCanvas/Libraries/Core/FunctionCallNode.generated.h>

namespace ScriptCanvas { class RuntimeComponent; }

namespace AZ
{
    class BehaviorMethod;
}

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            struct FunctionCallNodeCompareConfig;

            enum class IsFunctionCallNodeOutOfDateResult
            {
                No,
                Yes,
                EvaluateAfterLocalDefinition
            };

            class FunctionCallNode
                : public Node
                , AZ::Data::AssetBus::Handler
            {
                enum Version
                {
                    AddParserResults = 3,
                    RemoveMappingData,
                    CorrectAssetSubId,

                    // Add entry above
                    Current,
                };

            public:

                SCRIPTCANVAS_NODE(FunctionCallNode);

                FunctionCallNode();
                ~FunctionCallNode() override;

                void BuildNode();

                SubgraphInterfaceAsset* GetAsset()  const;

                AZ::Data::AssetId GetAssetId() const;

                const AZStd::string& GetAssetHint() const;

                const AZStd::string& GetName() const;

                Grammar::FunctionSourceId GetSourceId() const;

                void Initialize(AZ::Data::AssetId assetId, const ScriptCanvas::Grammar::FunctionSourceId& sourceId);

                bool IsOutOfDate(const VersionData& graphVersion) const override;

                IsFunctionCallNodeOutOfDateResult IsOutOfDate(const FunctionCallNodeCompareConfig& config, const AZ::Uuid& graphId) const;

                UpdateResult OnUpdateNode() override;

                AZStd::string GetUpdateString() const override;
                ////

                //////////////////////////////////////////////////////////////////////////
                // Translation
                

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                AZ::Outcome<Grammar::LexicalScope, void> GetFunctionCallLexicalScope(const Slot* slot) const override;

                AZ::Outcome<AZStd::string, void> GetFunctionCallName(const Slot* /*slot*/) const override;

                AZ::Outcome<AZStd::string, AZStd::string> GetInterfaceNameFromAssetOrLastSave() const;

                const SlotExecution::Map* GetSlotExecutionMap() const override;

                const Grammar::SubgraphInterface& GetSlotExecutionMapSource() const;

                const Grammar::SubgraphInterface* GetSubgraphInterface() const override;

                bool IsEntryPoint() const override;

                bool IsNodeableNode() const override;

                bool IsPure() const;

                bool IsSlotPure(const Slot* /*slot*/) const;
                //////////////////////////////////////////////////////////////////////////

            protected:
                SlotExecution::In AddAllSlots(const Grammar::In& in, int& slotOffset, const SlotExecution::Map& previousMap);
                SlotExecution::Out AddAllSlots(const Grammar::In& in, const Grammar::Out& out, int& slotOffset, const SlotExecution::Map& previousMap);
                SlotExecution::Out AddAllSlots(const Grammar::Out& latent, int& slotOffset, const SlotExecution::Map& previousMap);
                SlotExecution::In AddExecutionInSlotFromInterface(const Grammar::In& in, int slotOffset, SlotId previousSlotId);
                SlotExecution::Out AddExecutionOutSlotFromInterface(const Grammar::In& in, const Grammar::Out& out, int slotOffset, SlotId previousSlotId);
                SlotExecution::Out AddExecutionLatentOutSlotFromInterface(const Grammar::Out& latent, int slotOffset, SlotId previousSlotId);
                SlotExecution::Inputs AddDataInputSlotsFromInterface(const Grammar::Inputs& inputs, const Grammar::FunctionSourceId& inSourceId, const AZStd::string& displayGroup, const SlotExecution::Map& previousMap, int& slotOffset);
                SlotExecution::Outputs AddDataOutputSlotsFromInterface(const Grammar::Outputs& outputs, const AZStd::string& displayGroup, const SlotExecution::Map& previousMap, int& slotOffset);

                void BuildNodeFromSubgraphInterface
                    ( const AZ::Data::Asset<ScriptCanvas::SubgraphInterfaceAsset>& runtimeAsset
                    , const ScriptCanvas::Grammar::FunctionSourceId& sourceId
                    , const SlotExecution::Map& previousMap);

                void BuildUserFunctionCallNode
                    ( const Grammar::SubgraphInterface& subgraphInterface
                    , const ScriptCanvas::Grammar::FunctionSourceId& sourceId
                    , const SlotExecution::Map& previousMap);

                void BuildUserNodeableNode
                    ( const Grammar::SubgraphInterface& subgraphInterface
                    , const SlotExecution::Map& previousMap);

                void OnInit() override;

                // AssetBus...
                void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
                ///

                AZStd::string m_prettyName;

                Grammar::FunctionSourceId m_sourceId;
                AZ::Data::Asset<SubgraphInterfaceAsset> m_asset;
                SlotExecution::Map m_slotExecutionMap;
                Grammar::SubgraphInterface m_slotExecutionMapSourceInterface;

            private:

                struct DataSlotCache
                {
                    SlotId m_slotId;
                    VariableId m_variableReference;
                    Datum m_datum;
                };

                using ExecutionSlotMap = AZStd::unordered_map<Grammar::FunctionSourceId, SlotId>;
                using DataSlotMap = AZStd::unordered_map<VariableId, DataSlotCache>;

                void RemoveInsFromInterface(const Grammar::Ins& ins, ExecutionSlotMap& executionSlotMap, DataSlotMap& dataSlotMap, bool removeConnection, bool warnOnMissingSlot);
                void RemoveInsFromSlotExecution(const SlotExecution::Ins& ins, bool removeConnection, bool warnOnMissingSlot);

                void RemoveInputsFromInterface(const Grammar::Inputs& inputs, DataSlotMap& dataSlotMap, bool removeConnection, bool warnOnMissingSlot);
                void RemoveInputsFromSlotExecution(const SlotExecution::Inputs& inputs, bool removeConnection, bool warnOnMissingSlot);

                void RemoveOutsFromInterface(const Grammar::Outs& outs, ExecutionSlotMap& executionSlotMap, DataSlotMap& dataSlotMap, bool removeConnection, bool warnOnMissingSlot);
                void RemoveOutsFromSlotExecution(const SlotExecution::Outs& outs, bool removeConnection, bool warnOnMissingSlot);

                void RemoveOutputsFromInterface(const Grammar::Outputs& outputs, DataSlotMap& dataSlotMap, bool removeConnection, bool warnOnMissingSlot);
                void RemoveOutputsFromSlotExecution(const SlotExecution::Outputs& outputs, bool removeConnection, bool warnOnMissingSlot);

                void SanityCheckSlotsAndConnections(const ExecutionSlotMap& executionSlotMap, const DataSlotMap& dataSlotMap);
                void SanityCheckInSlotsAndConnections(const Graph& graph, const SlotExecution::Ins& ins, const ExecutionSlotMap& executionSlotMap, const DataSlotMap& dataSlotMap, ReplacementConnectionMap& connectionMap);
                void SanityCheckInputSlotsAndConnections(const Graph& graph, const SlotExecution::Inputs& inputs, const DataSlotMap& dataSlotMap, ReplacementConnectionMap& connectionMap);
                void SanityCheckOutSlotsAndConnections(const Graph& graph, const SlotExecution::Outs& outs, const ExecutionSlotMap& executionSlotMap, const DataSlotMap& dataSlotMap, ReplacementConnectionMap& connectionMap);
                void SanityCheckOutputSlotsAndConnections(const Graph& graph, const SlotExecution::Outputs& outputs, const DataSlotMap& dataSlotMap, ReplacementConnectionMap& connectionMap);
            };
        }
    }
}
