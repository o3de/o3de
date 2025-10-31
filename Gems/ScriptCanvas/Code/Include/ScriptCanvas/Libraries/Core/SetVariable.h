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

                const Slot* GetVariableInputSlot() const override;

                const Slot* GetVariableOutputSlot() const override;

                PropertyFields GetPropertyFields() const override;
                // Translation
                //////////////////////////////////////////////////////////////////////////


            protected:

                void OnInit() override;
                void OnPostActivate() override;

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
