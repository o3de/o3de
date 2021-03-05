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
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/containers/unordered_map.h>

#include <ScriptCanvas/CodeGen/CodeGen.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Asset/Functions/ScriptCanvasFunctionAsset.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Libraries/Core/ExecutionNode.h>
#include <ScriptCanvas/Libraries/Core/FunctionBus.h>

#include <Include/ScriptCanvas/Libraries/Core/FunctionNode.generated.h>

namespace ScriptCanvas { class RuntimeComponent; }

namespace ScriptCanvas { struct FunctionRuntimeData; }

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
            bool FunctionVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);

            using Namespaces = AZStd::vector<AZStd::string>;

            class FunctionNode
                : public Node
                , AZ::Data::AssetBus::Handler
                , FunctionRequestBus::Handler
                , private AZ::EntityBus::Handler
            {
            private:
                struct DataSlotIdCache
                {
                    AZ_TYPE_INFO(DataSlotIdCache, "{5E292B09-7B75-4F82-8F31-32B6AD15EA12}")

                    SlotId m_inputSlotId;
                    SlotId m_outputSlotId;

                    static void Reflect(AZ::ReflectContext* reflectContext);
                };

                typedef AZStd::unordered_map<AZ::Uuid, SlotId> ExecutionSlotMap;
                typedef AZStd::unordered_map<VariableId, DataSlotIdCache> DataSlotMap;

            public:

                ScriptCanvas_Node(FunctionNode,
                    ScriptCanvas_Node::Name("Function Node", "Displays a node that represents a function graph")
                    ScriptCanvas_Node::Uuid("{ECFDD30E-A16D-4435-97B7-B2A4DF3C543A}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Bus.png")
                    ScriptCanvas_Node::Version(3, FunctionVersionConverter)
                    ScriptCanvas_Node::DependentReflections(DataSlotIdCache)
                    ScriptCanvas_Node::DynamicSlotOrdering(true)
                    ScriptCanvas_Node::EditAttributes(AZ::Script::Attributes::ExcludeFrom(AZ::Script::Attributes::ExcludeFlags::All))
                );

                FunctionNode();
                ~FunctionNode() override;

                ScriptCanvas_SerializeProperty(AZ::Data::Asset<RuntimeFunctionAsset>, m_asset);
                ScriptCanvas_SerializePropertyWithDefaults(size_t, m_savedFunctionVersion, 0);

                ScriptCanvas_SerializeProperty(ExecutionSlotMap, m_executionSlotMapping);
                ScriptCanvas_SerializeProperty(DataSlotMap, m_dataSlotMapping);

                RuntimeFunctionAsset* GetAsset()  const;
                AZ::Data::AssetId GetAssetId() const;

                void ConfigureNode(const AZ::Data::AssetId& assetId);

                void BuildNode();

                const AZStd::string& GetName() const { return m_prettyName; }

                void Initialize(AZ::Data::AssetId assetId);

                // NodeVersioning
                bool IsOutOfDate() const override;
                UpdateResult OnUpdateNode() override;
                AZStd::string GetUpdateString() const override;
                ////

            protected:

                void BuildNodeImpl(const AZ::Data::Asset<ScriptCanvas::RuntimeFunctionAsset>& runtimeAsset, ExecutionSlotMap& executionMapping, DataSlotMap& dataSlotMapping);

                void OnInit() override;
                void OnDeactivate() override;

                void CompleteDeactivation();

                void OnInputSignal(const SlotId&) override;
                void OnSignalOut(ID nodeId, SlotId slotId) override;

                void PushDataOut();

                size_t m_currentFunctionVersion;

                AZStd::recursive_mutex m_mutex;
                FunctionRuntimeData* m_runtimeData;

                using EntryPointContainer = AZStd::unordered_map<SlotId, Node*>;
                EntryPointContainer m_entryPoints;

                using ExitPointContainer = AZStd::unordered_map<ID, SlotId>;
                ExitPointContainer m_exitPoints;

                // AssetBus
                void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

                //! Handle the OnEntity destruction event for the m_executationContextEntity
                //! for the scenario when this function is component is destroyed due to closing
                //! the game. In that case The ComponentApplication Destroys all entities
                //! in an undefined order
                void OnEntityDestruction(const AZ::EntityId& entityId) override;

                AZStd::string m_prettyName;

                AZStd::unique_ptr<AZ::Entity> m_executionContextEntity;
                RuntimeComponent* m_runtimeComponent;

                bool m_setupFunction = false;

                using DataSlotVariableMap = AZStd::unordered_map<SlotId, GraphVariable*>;
                DataSlotVariableMap m_inputSlots;
                DataSlotVariableMap m_outputSlots;

                using ExecutionSlotIdMap = AZStd::unordered_map<SlotId, AZ::Uuid>;
                ExecutionSlotIdMap m_executionInputMapping;
                ExecutionSlotIdMap m_executionOutputMapping;
            };
        }
    }
}
