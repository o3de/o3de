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

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string_view.h>

#include <ScriptCanvas/Core/Contracts/SlotTypeContract.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Core/SlotConfigurations.h>
#include <ScriptCanvas/Core/ModifiableDatumView.h>
#include <ScriptCanvas/Data/Data.h>

#include <ScriptCanvas/Variable/VariableCore.h>
#include <ScriptCanvas/Variable/VariableBus.h>

namespace ScriptCanvas
{
    class Contract;
    class Node;

    struct TransientSlotIdentifier
    {
        AZStd::string m_name;
        SlotDescriptor m_slotDescriptor;

        int m_index = 0;
    };
    
    class Slot final
        : public VariableNotificationBus::Handler
    {
        friend class Node;
    public:

        enum class DataType : AZ::s32
        {
            NoData,
            Data,
            VariableReference
        };

        AZ_CLASS_ALLOCATOR(Slot, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(Slot, "{FBFE0F02-4C26-475F-A28B-18D3A533C13C}");

        static void Reflect(AZ::ReflectContext* reflection);

        Slot() = default;
        Slot(const Slot& slot);
        Slot(Slot&& slot);

        Slot(const SlotConfiguration& slotConfiguration);

        ~Slot();

        Slot& operator=(const Slot& slot);

        void AddContract(const ContractDescriptor& contractDesc);

        template<typename T>
        void RemoveContract()
        {
            AZ::Uuid contractType = azrtti_typeid<T>();

            auto contractIter = m_contracts.begin();

            while (contractIter != m_contracts.end())
            {
                if (azrtti_typeid(contractIter->get()) == contractType)
                {
                    m_contracts.erase(contractIter);
                    break;
                }

                ++contractIter;
            }
        }

        AZStd::vector<AZStd::unique_ptr<Contract>>& GetContracts() { return m_contracts; }
        const AZStd::vector<AZStd::unique_ptr<Contract>>& GetContracts() const { return m_contracts; }

        // ConvertToLatentExecutionOut
        //
        // Mainly here to limit scope of what manipulation can be done to the slots. We need to version convert the slots
        // but at a higher tier, so instead of allowing the type to be set, going to just make this specific function which does
        // the conversion we are after.
        void ConvertToLatentExecutionOut();
        ////

        const SlotDescriptor& GetDescriptor() const { return m_descriptor; }
        const SlotId& GetId() const { return m_id; }

        const Node* GetNode() const { return m_node; }
        Node* GetNode() { return m_node; }

        Endpoint GetEndpoint() const;
        AZ::EntityId GetNodeId() const;

        void SetNode(Node* node);
        void InitializeVariables();

        const AZStd::string& GetName() const { return m_name; }
        const AZStd::string& GetToolTip() const { return m_toolTip; }

        Data::Type GetDataType() const;

        bool IsConnected() const;

        bool IsData() const;

        const Datum* FindDatum() const;
        void FindModifiableDatumView(ModifiableDatumView& datumView);

        // If you are data. You could be a reference pin(i.e. must be a variable)
        // Or a value data pin.
        bool IsVariableReference() const;

        bool CanConvertTypes() const;

        bool CanConvertToValue() const;
        bool ConvertToValue();

        bool CanConvertToReference() const;
        bool ConvertToReference();
        void SetVariableReference(const VariableId& variableId);
        const VariableId& GetVariableReference() const;
        GraphVariable* GetVariable() const;

        void ClearVariableReference();

        bool IsExecution() const;

        bool IsInput() const;
        bool IsOutput() const;
        ScriptCanvas::ConnectionType GetConnectionType() const;

        bool IsLatent() const;

        // VariableNotificationBus
        void OnVariableValueChanged() override;
        ////

        // Here to allow conversion of the previously untyped any slots into the dynamic type any.
        void SetDynamicDataType(DynamicDataType dynamicDataType);
        ////

        const DynamicDataType& GetDynamicDataType() const { return m_dynamicDataType; }
        bool IsDynamicSlot() const;

        void SetDisplayType(Data::Type displayType);
        void ClearDisplayType();
        Data::Type GetDisplayType() const;
        bool HasDisplayType() const;

        AZ::Crc32 GetDisplayGroup() const;

        // Should only be used for updating slots. And never really done at runtime as slots
        // won't be re-arranged.
        void SetDisplayGroup(AZStd::string displayGroup);

        AZ::Crc32 GetDynamicGroup() const;

        AZ::Outcome<void, AZStd::string> IsTypeMatchFor(const Slot& slot) const;
        AZ::Outcome<void, AZStd::string> IsTypeMatchFor(const Data::Type& dataType) const;

        void Rename(AZStd::string_view slotName);

        void SignalRenamed();
        void SignalTypeChanged(const ScriptCanvas::Data::Type& dataType);

        void UpdateDatumVisibility();

        // Editor Fields

        // Returns information which can be used to identify this slot in a 'transient' fashion.
        // This data should not be stored and used for long term retrieval but should be valid within a single session
        // to identify the same slot between different nodes.
        TransientSlotIdentifier GetTransientIdentifier() const;
        ////

    protected:

        void SetDynamicGroup(const AZ::Crc32& dynamicGroup);

        AZStd::string m_name;
        AZStd::string m_toolTip;
        AZ::Crc32 m_displayGroup;
        AZ::Crc32 m_dynamicGroup;

        bool               m_isLatentSlot  = false;
        SlotDescriptor     m_descriptor;

        bool               m_isVariableReference = false;
        DataType           m_dataType = DataType::NoData;

        VariableId         m_variableReference;
        GraphVariable*     m_variable = nullptr;

        DynamicDataType m_dynamicDataType{ DynamicDataType::None };
        ScriptCanvas::Data::Type m_displayDataType{ ScriptCanvas::Data::Type::Invalid() };

        SlotId m_id;
        Node*  m_node;

        AZStd::vector<AZStd::unique_ptr<Contract>> m_contracts;
    };
} // namespace ScriptCanvas
