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

#include <ScriptCanvas/CodeGen/CodeGen.h>
#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/SlotMetadata.h>
#include <ScriptCanvas/Core/SlotNames.h>
#include <ScriptCanvas/Data/PropertyTraits.h>

#include <Include/ScriptCanvas/Libraries/Core/ForEach.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            class ForEach
                : public Node
                , public EndpointNotificationBus::Handler
            {
            public:
                ScriptCanvas_Node(ForEach,
                    ScriptCanvas_Node::Name("For Each", "Node for iterating through a container")
                    ScriptCanvas_Node::Uuid("{31823035-A3FF-4B8D-A3A6-819519478B7C}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Placeholder.png")
                    ScriptCanvas_Node::Category("Containers")
                    ScriptCanvas_Node::Version(2, VersionConverter)
                );

                static bool VersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);

                Data::Type GetSourceSlotDataType() const;
                AZStd::vector<AZStd::pair<AZStd::string_view, SlotId>> GetPropertyFields() const;

            protected:
                void OnInit() override;
                void OnInputSignal(const SlotId&) override;

                bool InitializeLoop();
                void Iterate();
                bool SetPropertySlotData(Datum& atResult, size_t propertyIndex);
                void ResetLoop();

                void OnDynamicGroupDisplayTypeChanged(const AZ::Crc32& dynamicGroup, const Data::Type& dataType) override;

                void ClearPropertySlots();
                void AddPropertySlotsFromType(const Data::Type& dataType);

                static AZ::Crc32 GetContainerGroupId() { return AZ_CRC("ContainerGroup", 0xb81ed451); }

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "Signaled upon node entry"));
                ScriptCanvas_In(ScriptCanvas_In::Name("Break", "Stops the iteration when signaled"));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Each", "Signalled after each element of the container"));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Finished", "The container has been fully iterated over"));

                // Properties
                ScriptCanvas_SerializeProperty(SlotId, m_sourceSlot);
                ScriptCanvas_SerializeProperty(AZ::TypeId, m_previousTypeId);
                ScriptCanvas_SerializeProperty(AZStd::vector<Data::PropertyMetadata>, m_propertySlots);

                static const size_t k_keySlotIndex;
                static const size_t k_valueSlotIndex;

                size_t m_index;
                size_t m_size;

                bool m_breakCalled;

                Datum m_sourceContainer;
                Datum m_keysVector;
            };
        }
    }
}
