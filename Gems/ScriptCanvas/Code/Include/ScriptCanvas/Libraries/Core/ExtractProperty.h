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

#include <Include/ScriptCanvas/Libraries/Core/ExtractProperty.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            class ExtractProperty
                : public Node
            {
            public:
                ScriptCanvas_Node(ExtractProperty,
                    ScriptCanvas_Node::Name("Extract Properties", "Extracts property values from connected input")
                    ScriptCanvas_Node::Uuid("{D4C9DA8E-838B-41C6-B870-C75294C323DC}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Placeholder.png")
                    ScriptCanvas_Node::Category("Utilities")
                    ScriptCanvas_Node::Version(1, VersionConverter)
                );

                static bool VersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);

                Data::Type GetSourceSlotDataType() const;
                AZStd::vector<AZStd::pair<AZStd::string_view, SlotId>> GetPropertyFields() const;

                // Node
                bool IsOutOfDate() const override;
                ////

            protected:
                void OnInit() override;

                UpdateResult OnUpdateNode() override;

                void OnInputSignal(const SlotId&) override;

                void OnSlotDisplayTypeChanged(const SlotId& slotId, const Data::Type& dataType) override;

                void AddPropertySlots(const Data::Type& type);
                void ClearPropertySlots();

                void RefreshGetterFunctions();

                ScriptCanvas_In(ScriptCanvas_In::Name("In", "When signaled assigns property values using the supplied source input"));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", "Signaled after all property haves have been pushed to the output slots"));

                ScriptCanvas_DynamicDataSlot(ScriptCanvas::DynamicDataType::Value,
                    ScriptCanvas::ConnectionType::Input,
                    ScriptCanvas_DynamicDataSlot::Name("Source", "The value on which to extract properties from.")
                )

                ScriptCanvas_SerializeProperty(Data::Type, m_dataType);
                ScriptCanvas_SerializeProperty(AZStd::vector<Data::PropertyMetadata>, m_propertyAccounts);

                friend class ExtractPropertyEventHandler;

            private:

                void UpdatePropertyVersion();

                void AddPropertySlots(const Data::Type& dataType, const AZStd::unordered_map<AZStd::string, SlotId>& versionedSlots);
            };
        }
    }
}
