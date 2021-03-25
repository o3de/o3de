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
            //! Provides a node to set the value of a variable
            class SetVariableNode
                : public Node
                , protected VariableNotificationBus::Handler
                , protected VariableNodeRequestBus::Handler
            {
            public:

                SCRIPTCANVAS_NODE(SetVariableNode);

                // Node...
                void CollectVariableReferences(AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const override;
                bool ContainsReferencesToVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const override;
                bool RemoveVariableReferences(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) override;
                ////

                //// VariableNodeRequestBus...
                void SetId(const VariableId& variableId) override;
                const VariableId& GetId() const override;
                ////

                const SlotId& GetDataInSlotId() const;
                const SlotId& GetDataOutSlotId() const;

                //////////////////////////////////////////////////////////////////////////
                // Translation
                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                VariableId GetVariableIdRead(const Slot*) const override;

                VariableId GetVariableIdWritten(const Slot*) const override;

                const Slot* GetVariableOutputSlot() const override;

                PropertyFields GetPropertyFields() const override;
                // Translation
                //////////////////////////////////////////////////////////////////////////

                bool IsSupportedByNewBackend() const override { return true; }

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

                VariableId m_variableId;

                SlotId m_variableDataInSlotId;
                SlotId m_variableDataOutSlotId;

                AZStd::vector<Data::PropertyMetadata> m_propertyAccounts;

                AZStd::string_view  m_variableName;
                ModifiableDatumView m_variableView;

            };
        }
    }
}
