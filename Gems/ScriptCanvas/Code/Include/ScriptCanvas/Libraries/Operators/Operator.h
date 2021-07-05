/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_map.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/SlotMetadata.h>

#include <Include/ScriptCanvas/Libraries/Operators/Operator.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            class OperatorBase 
                : public ScriptCanvas::Node
            {
            public:

                SCRIPTCANVAS_NODE(OperatorBase);

                typedef AZStd::vector<const Datum*> OperatorOperands;

                enum class SourceType
                {
                    SourceInput,
                    SourceOutput
                };

                struct SourceSlotConfiguration
                {
                    SourceType m_sourceType = SourceType::SourceInput;

                    DynamicDataType m_dynamicDataType  = DynamicDataType::Any;

                    AZStd::string m_name;
                    AZStd::string m_tooltip;
                };

                struct OperatorConfiguration
                {
                    AZStd::vector< SourceSlotConfiguration > m_sourceSlotConfigurations;
                };

                void CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const override;

                using TypeList = AZStd::vector<AZ::TypeId>;
                using SlotSet = AZStd::unordered_set<SlotId>;

                SlotSet m_sourceSlots;

                // Contains the ScriptCanvas data types used for display and other state control
                ScriptCanvas::Data::Type m_sourceType;
                ScriptCanvas::Data::Type m_sourceDisplayType;

                // Has all of the internal AZ types that may be apart of the source type(i.e. for containers)
                TypeList m_sourceTypes;

                SlotSet m_outputSlots;
                SlotSet m_inputSlots;

                OperatorBase();
                OperatorBase(const OperatorConfiguration& c);

                bool IsSourceSlotId(const SlotId& slotId) const;
                const SlotSet& GetSourceSlots() const;

                ScriptCanvas::Data::Type GetSourceType() const;
                AZ::TypeId GetSourceAZType() const;

                ScriptCanvas::Data::Type GetDisplayType() const;

            protected:

                AZ::Crc32 GetSourceDynamicTypeGroup() const { return AZ::Crc32("OperatorGroup"); }
                AZStd::string GetSourceDisplayGroup() const { return "OperatorGroup"; }

                void OnInit() override;
                void OnDynamicGroupDisplayTypeChanged(const AZ::Crc32& dynamicGroup, const Data::Type& dataType) override final;
                void OnSlotRemoved(const SlotId& slotId) override;

                Slot* GetFirstInputSourceSlot() const;
                Slot* GetFirstOutputSourceSlot() const;

                virtual SlotId AddSourceSlot(SourceSlotConfiguration sourceConfiguration);
                virtual void ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs);

                void RemoveInputs();
                void RemoveOutputs();

                void OnEndpointConnected(const Endpoint& endpoint) override;
                void OnEndpointDisconnected(const Endpoint& endpoint) override;

                virtual void OnInputSlotAdded(const SlotId& inputSlotId) { AZ_UNUSED(inputSlotId); };
                virtual void OnDataInputSlotConnected([[maybe_unused]] const SlotId& slotId, [[maybe_unused]] const Endpoint& endpoint) {}
                virtual void OnDataInputSlotDisconnected([[maybe_unused]] const SlotId& slotId, [[maybe_unused]] const Endpoint& endpoint) {}                

                AZ::BehaviorMethod* GetOperatorMethod(const char* methodName);

                SlotId AddSlotWithSourceType();

                bool HasSourceConnection() const;

                bool AreSourceSlotsFull(SourceType sourceType) const;

                void PopulateAZTypes(ScriptCanvas::Data::Type dataType);

                //! Called when the source data type of the operator has changed, it is used to mutate the node's topology into the desired type
                virtual void OnSourceTypeChanged() {}
                virtual void OnDisplayTypeChanged([[maybe_unused]] ScriptCanvas::Data::Type dataType) {}
                virtual void OnSourceConnected([[maybe_unused]] const SlotId& slotId) {}
                virtual void OnSourceDisconnected([[maybe_unused]] const SlotId& slotId) {}

                //! Implements the operator's behavior, the vector of Datums represents the list of operands.
                virtual void Evaluate(const OperatorOperands& operands, Datum& result)
                {
                    AZ_UNUSED(operands);
                    AZ_UNUSED(result);
                }

            private:
                OperatorConfiguration m_operatorConfiguration;

                AZ::TypeId m_sourceTypeId;
            };

            struct DefaultContainerManipulationOperatorConfiguration
                : public OperatorBase::OperatorConfiguration
            {
                DefaultContainerManipulationOperatorConfiguration()
                {
                    {
                        OperatorBase::SourceSlotConfiguration containerSourceConfig;

                        containerSourceConfig.m_dynamicDataType = DynamicDataType::Container;
                        containerSourceConfig.m_name = "Source";
                        containerSourceConfig.m_tooltip = "The source object to operator on.";
                        containerSourceConfig.m_sourceType = OperatorBase::SourceType::SourceInput;

                        m_sourceSlotConfigurations.emplace_back(containerSourceConfig);
                    }

                    {
                        OperatorBase::SourceSlotConfiguration containerSourceConfig;

                        containerSourceConfig.m_dynamicDataType = DynamicDataType::Container;
                        containerSourceConfig.m_name = "Container";
                        containerSourceConfig.m_tooltip = "The container that was operated upon.";
                        containerSourceConfig.m_sourceType = OperatorBase::SourceType::SourceOutput;

                        m_sourceSlotConfigurations.emplace_back(containerSourceConfig);
                    }
                }
            };

            struct DefaultContainerInquiryOperatorConfiguration
                : public OperatorBase::OperatorConfiguration
            {
                DefaultContainerInquiryOperatorConfiguration()
                {
                    {
                        OperatorBase::SourceSlotConfiguration containerSourceConfig;

                        containerSourceConfig.m_dynamicDataType = DynamicDataType::Container;
                        containerSourceConfig.m_name = "Source";
                        containerSourceConfig.m_tooltip = "The source object to operator on.";
                        containerSourceConfig.m_sourceType = OperatorBase::SourceType::SourceInput;

                        m_sourceSlotConfigurations.emplace_back(containerSourceConfig);
                    }
                }
            };

            // Base class for an small helper object that wraps the function to invoke
            class OperationHelper
            {
            public:

                Datum operator()(const AZStd::vector<Datum>& operands, Datum& result)
                {
                    AZStd::vector<Datum>::const_iterator operand = operands.begin();
                    result = *operand;
                    for (++operand; operand != operands.end(); ++operand)
                    {
                        result = Operator(result, *operand);
                    }

                    return result;
                }

            protected:
                virtual Datum Operator(const Datum&, const Datum&) = 0;
            };

// Helper macro to invoke an operator function for a specialized type
#define CallOperatorFunction(Operator, DataType, Type) \
    if (DataType == Data::FromAZType(azrtti_typeid<Type>())) {  Operator<Type> operation; operation(operands, result); }

        }
    }
}
