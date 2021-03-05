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

#include <AzCore/Serialization/EditContextConstants.inl>

#include <ScriptCanvas/CodeGen/CodeGen.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/SlotNames.h>
#include <ScriptCanvas/Data/PropertyTraits.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Variable/VariableCore.h>

#include <Include/ScriptCanvas/Libraries/Core/SetVariable.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            class SetVariableNode
                : public Node
                , protected VariableNotificationBus::Handler
                , protected VariableNodeRequestBus::Handler
            {
            public:
                ScriptCanvas_Node(SetVariableNode,
                    ScriptCanvas_Node::Name("Set Variable", "Node for setting a property within the graph")
                    ScriptCanvas_Node::Uuid("{5EFD2942-AFF9-4137-939C-023AEAA72EB0}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Placeholder.png")
                    ScriptCanvas_Node::Version(1)
                );

                // Node
                void CollectVariableReferences(AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const override;
                bool ContainsReferencesToVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const override;
                bool RemoveVariableReferences(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) override;
                ////

                //// VariableNodeRequestBus
                void SetId(const VariableId& variableId) override;
                const VariableId& GetId() const override;
                ////

                const SlotId& GetDataInSlotId() const;
                const SlotId& GetDataOutSlotId() const;

            protected:

                void OnInit() override;
                void OnPostActivate() override;                
                void OnInputSignal(const SlotId&) override;

                void AddSlots();
                void RemoveSlots();
                void AddPropertySlots(const Data::Type& type);
                void ClearPropertySlots();
                void RefreshPropertyFunctions();

                GraphScopedVariableId GetScopedVariableId() const;

                GraphVariable* ModVariable() const;

                void OnIdChanged(const VariableId& oldVariableId);
                AZStd::vector<AZStd::pair<VariableId, AZStd::string>> GetGraphVariables() const;

                // VariableNotificationBus
                void OnVariableRemoved() override;
                ////

                AnnotateNodeSignal CreateAnnotationData();

                ScriptCanvas_In(ScriptCanvas_In::Name("In", "When signaled sends the variable referenced by this node to a Data Output slot"));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", "Signaled after the referenced variable has been pushed to the Data Output slot"));

                ScriptCanvas_EditPropertyWithDefaults(VariableId, m_variableId, , EditProperty::NameLabelOverride("Variable Name"),
                    EditProperty::DescriptionTextOverride("Name of ScriptCanvas Variable"),
                    EditProperty::UIHandler(AZ::Edit::UIHandlers::ComboBox),
                    EditProperty::EditAttributes(AZ::Edit::Attributes::GenericValueList(&SetVariableNode::GetGraphVariables), AZ::Edit::Attributes::PostChangeNotify(&SetVariableNode::OnIdChanged)));

                ScriptCanvas_SerializeProperty(SlotId, m_variableDataInSlotId);
                ScriptCanvas_SerializeProperty(SlotId, m_variableDataOutSlotId);

                ScriptCanvas_SerializeProperty(AZStd::vector<Data::PropertyMetadata>, m_propertyAccounts);

                AZStd::string_view  m_variableName;
                ModifiableDatumView m_variableView;

            };
        }
    }
}
