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

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/NodelingBus.h>
#include <ScriptCanvas/CodeGen/CodeGen.h>

#include <Include/ScriptCanvas/Libraries/Core/ExecutionNode.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            namespace Internal
            {
                class Nodeling
                    : public Node
                    , public NodePropertyInterfaceListener
                    , public NodelingRequestBus::Handler
                {
                private:
                    enum NodeVersion
                    {
                        Initial = 1,
                    };

                public:
                    ScriptCanvas_Node(Nodeling,
                        ScriptCanvas_Node::Name("Nodeling", "Represents either an execution entry or exit node")
                        ScriptCanvas_Node::Uuid("{4413EEA0-8D81-4D61-A1E1-3C1A437F3643}")
                        ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Start.png")
                        ScriptCanvas_Node::Version(1)
                        ScriptCanvas_Node::Category("Core")
                    );

                    Nodeling();
                    ~Nodeling() = default;

                    AZStd::string GetSlotDisplayGroup() const { return "NodelingSlotDisplayGroup"; }
                    AZ::Crc32 GetSlotDisplayGroupId() const { return AZ_CRC("NodelingSlotDisplayGroup", 0xedf94173); }

                    AZ::Crc32 GetPropertyId() const { return AZ_CRC("NodeNameProperty", 0xe967a10a); }

                    const AZ::Uuid& GetIdentifier() const { return m_identifier; }

                    // Node
                    void OnInit() override;
                    void OnGraphSet() override;

                    void ConfigureVisualExtensions() override;

                    NodePropertyInterface* GetPropertyInterface(AZ::Crc32 propertyId) override;
                    ////

                    // NodelingRequestBus
                    AZ::EntityId GetNodeId() const override { return GetEntityId(); }
                    GraphScopedNodeId GetGraphScopedNodeId() const override { return GetScopedNodeId(); }
                    const AZStd::string& GetDisplayName() const override { return m_displayName; }

                    void SetDisplayName(const AZStd::string& displayName) override;
                    ////

                    void RemapId();

                protected:

                    AZStd::string m_previousName;

                    ScriptCanvas_SerializeProperty(Data::StringType, m_displayName);
                    ScriptCanvas_SerializeProperty(AZ::Uuid, m_identifier);

                private:
                    // NodePropertyInterface
                    void OnPropertyChanged() override;
                    ////

                    TypedNodePropertyInterface<ScriptCanvas::Data::StringType> m_displayNameInterface;
                };
            }

            class ExecutionNodeling
                : public Internal::Nodeling
            {
            private:
                enum NodeVersion
                {
                    Initial  = 1
                };

            public:
                ScriptCanvas_Node(ExecutionNodeling,
                    ScriptCanvas_Node::Name("Execution Nodeling", "Represents either an execution entry or exit node.")
                    ScriptCanvas_Node::Uuid("{4EE28D9F-67FB-4E61-B777-5DC5B059710F}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Start.png")
                    ScriptCanvas_Node::Version(1)
                    ScriptCanvas_Node::Category("Core")
                    ScriptCanvas_Node::EditAttributes(AZ::Script::Attributes::ExcludeFrom(AZ::Script::Attributes::ExcludeFlags::All))
                );

                ExecutionNodeling() = default;
                ~ExecutionNodeling() = default;

                ConnectionType GetConnectionType() const { return m_externalConnectionType; }

                // Node
                bool IsEntryPoint() const override;

                void OnConfigured() override;
                void OnActivate() override;

                void OnInputSignal(const SlotId&) override;
                ////

                AZStd::vector<const ScriptCanvas::Slot*> GetEntrySlots() const;

                void OnEndpointConnected(const ScriptCanvas::Endpoint& endpoint) override;
                void OnEndpointDisconnected(const ScriptCanvas::Endpoint& endpoint) override;

                void SignalEntrySlots();

            protected:

                void ConfigureExternalConnectionType();
                
                ScriptCanvas_SerializePropertyWithDefaults(ConnectionType, m_externalConnectionType, ConnectionType::Unknown);

            private:

                void SetupSlots();
                
                AZStd::vector<const ScriptCanvas::Slot*> m_entrySlots;
            };
        }
    }
}
