/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/EditContextConstants.inl>

#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Data/PropertyTraits.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Variable/VariableCore.h>

#include <Include/ScriptCanvas/Libraries/Core/GetVariable.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            //! Provides a node for retreiving the value of a variable
            class GetVariableNode
                : public Node
                , protected VariableNotificationBus::Handler
                , protected VariableNodeRequestBus::Handler
            {
            public:

                SCRIPTCANVAS_NODE(GetVariableNode);

                // Node
                void CollectVariableReferences(AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const override;
                bool ContainsReferencesToVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const override;
                bool RemoveVariableReferences(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) override;
                ////

                //// VariableNodeRequestBus
                void SetId(const VariableId& variableId) override;
                const VariableId& GetId() const override;
                //// 
                const Datum* GetDatum() const;

                const SlotId& GetDataOutSlotId() const;

                //////////////////////////////////////////////////////////////////////////
                // Translation
                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                PropertyFields GetPropertyFields() const override;

                VariableId GetVariableIdRead(const Slot*) const override;

                const Slot* GetVariableOutputSlot() const override;
                // Translation
                //////////////////////////////////////////////////////////////////////////


            protected:

                void OnInit() override;
                void OnPostActivate() override;

                void AddOutputSlot();
                void RemoveOutputSlot();

                GraphScopedVariableId GetScopedVariableId() const;

                AZStd::vector<AZStd::pair<VariableId, AZStd::string>> GetGraphVariables() const;
                void OnIdChanged(const VariableId& oldVariableId);

                // VariableNotificationBus
                void OnVariableRemoved() override;
                ////

                // Adds/Remove Property Slots from the GetVariable node
                void AddPropertySlots(const Data::Type& type);
                void ClearPropertySlots();

                void RefreshPropertyFunctions();

                VariableId m_variableId;
                SlotId m_variableDataOutSlotId;
                AZStd::vector<Data::PropertyMetadata> m_propertyAccounts;

                AZStd::string_view m_variableName;
                ModifiableDatumView m_variableView;
            };
        }
    }
}
