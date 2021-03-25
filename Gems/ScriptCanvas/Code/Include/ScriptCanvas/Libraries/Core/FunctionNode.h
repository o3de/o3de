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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>
#include <ScriptCanvas/Asset/Functions/ScriptCanvasFunctionAsset.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/SubgraphInterface.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/SlotExecutionMap.h>
#include <ScriptCanvas/Libraries/Core/FunctionDefinitionNode.h>
#include <ScriptCanvas/Utils/VersioningUtils.h>
#include <Include/ScriptCanvas/Libraries/Core/FunctionNode.generated.h>

namespace ScriptCanvas { class RuntimeComponent; }

namespace ScriptCanvas { struct SubgraphInterfaceData; }

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
            class FunctionNode
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

                SCRIPTCANVAS_NODE(FunctionNode);

                FunctionNode();
                ~FunctionNode() override;

                void BuildNode();

                void ConfigureNode(const AZ::Data::AssetId& assetId);

                SubgraphInterfaceAsset* GetAsset()  const;

                AZ::Data::AssetId GetAssetId() const;

                const AZStd::string& GetName() const;

                void Initialize(AZ::Data::AssetId assetId);

                // NodeVersioning
                bool IsOutOfDate(const VersionData& graphVersion) const override;

                UpdateResult OnUpdateNode() override;

                AZStd::string GetUpdateString() const override;
                ////

                //////////////////////////////////////////////////////////////////////////
                // Translation
                bool IsSupportedByNewBackend() const override { return true; }

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                AZ::Outcome<Grammar::LexicalScope, void> GetFunctionCallLexicalScope(const Slot* slot) const override;

                AZ::Outcome<AZStd::string, void> GetFunctionCallName(const Slot* /*slot*/) const override;

                AZStd::string GetInterfaceName() const;

                const SlotExecution::Map* GetSlotExecutionMap() const override;

                const Grammar::SubgraphInterface* GetSubgraphInterface() const override;

                bool IsNodeableNode() const override;

                bool IsPure() const;

                bool IsSlotPure(const Slot* /*slot*/) const;
                //////////////////////////////////////////////////////////////////////////

            protected:
                SlotExecution::In AddExecutionInSlotFromInterface(const Grammar::In& in, int slotOffset, SlotId previousSlotId);
                SlotExecution::Out AddExecutionOutSlotFromInterface(const Grammar::In& in, const Grammar::Out& out, int slotOffset, SlotId previousSlotId);
                SlotExecution::Out AddExecutionLatentOutSlotFromInterface(const Grammar::Out& latent, int slotOffset, SlotId previousSlotId);
                SlotExecution::Inputs AddDataInputSlotFromInterface(const Grammar::Inputs& inputs, const Grammar::FunctionSourceId& inSourceId, const AZStd::string& displayGroup, const SlotExecution::Map& previousMap, int& slotOffset);
                SlotExecution::Outputs AddDataOutputSlotFromInterface(const Grammar::Outputs& outputs, const AZStd::string& displayGroup, const SlotExecution::Map& previousMap, int& slotOffset);

                void BuildNodeFromSubgraphInterface(const AZ::Data::Asset<ScriptCanvas::SubgraphInterfaceAsset>& runtimeAsset, const SlotExecution::Map& previousMap);

                void OnInit() override;

                // AssetBus...
                void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
                ///

                AZStd::string m_prettyName;

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
