/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    struct SlotState
    {
        CombinedSlotType type;
        AZStd::string name;
        VariableId variableReference;
        Datum value;
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

        AZ_CLASS_ALLOCATOR(Slot, AZ::SystemAllocator);
        AZ_TYPE_INFO(Slot, "{FBFE0F02-4C26-475F-A28B-18D3A533C13C}");

        static void Reflect(AZ::ReflectContext* reflection);

        Slot() = default;
        Slot(const Slot& slot);
        Slot(Slot&& slot);

        Slot(const SlotConfiguration& slotConfiguration);

        ~Slot();

        Slot& operator=(const Slot& slot);

        void AddContract(const ContractDescriptor& contractDesc);

        void ClearDynamicGroup();

        template<typename T>
        T* FindContract()
        {
            AZ::Uuid contractType = azrtti_typeid<T>();

            for (const auto& contractPtr : m_contracts)
            {
                if (azrtti_typeid(contractPtr.get()) == contractType)
                {
                    return azrtti_cast<T*>(contractPtr.get());
                }
            }

            return nullptr;
        }

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

        CombinedSlotType GetType() const;
        
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
        bool FindModifiableDatumView(ModifiableDatumView& datumView);

        // If you are data. You could be a reference pin(i.e. must be a variable)
        // Or a value data pin.
        bool IsVariableReference() const;

        bool CanHaveInputField() const;

        bool CreatesImplicitConnections() const;

        bool IsNameHidden() const;

        bool CanConvertTypes() const;

        bool CanConvertToValue() const;
        bool ConvertToValue();

        bool CanConvertToReference(bool isNewSlot = false) const;
        bool ConvertToReference(bool isNewSlot = false);
        enum class IsVariableTypeChange
        {
            No,
            Yes
        };
        void SetVariableReference(const VariableId& variableId, IsVariableTypeChange isTypeChange = IsVariableTypeChange::No);
        const VariableId& GetVariableReference() const;
        GraphVariable* GetVariable() const;

        void ClearVariableReference();

        bool IsExecution() const;
        
        bool IsVisible() const;
        bool IsUserAdded() const;

        void SetVisible(bool isVisible);

        bool IsInput() const;
        bool IsOutput() const;
        ScriptCanvas::ConnectionType GetConnectionType() const;

        bool IsLatent() const;

        // Here to allow conversion of the previously untyped any slots into the dynamic type any.
        void SetDynamicDataType(DynamicDataType dynamicDataType);

        const DynamicDataType& GetDynamicDataType() const { return m_dynamicDataType; }
        bool IsDynamicSlot() const;

        void SetDisplayType(Data::Type displayType);
        void ClearDisplayType();
        Data::Type GetDisplayType() const;
        bool HasDisplayType() const;
        bool IsSanityCheckRequired() const;

        AZ::Crc32 GetDisplayGroup() const;

        // Should only be used for updating slots. And never really done at runtime as slots
        // won't be re-arranged.
        void SetDisplayGroup(AZStd::string displayGroup);

        AZ::Crc32 GetDynamicGroup() const;

        AZ::Outcome<void, AZStd::string> IsTypeMatchFor(const Slot& slot) const;
        AZ::Outcome<void, AZStd::string> IsTypeMatchFor(const Data::Type& dataType) const;

        // Doesn't actually push the new tooltip out to the UI. So any updates need to be done
        // before any visuals are created.
        void SetToolTip(const AZStd::string& toolTip);

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
        bool m_isOverload = false;
        bool m_isVisible = true;
        bool m_isUserAdded = false;
        bool m_createsImplicitConnections = false;

        void SetDynamicGroup(const AZ::Crc32& dynamicGroup);

        AZStd::string m_name;
        AZStd::string m_toolTip;
        AZ::Crc32 m_displayGroup;
        AZ::Crc32 m_dynamicGroup;

        bool m_canHaveInputField = true;

        bool m_isNameHidden = false;

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

        bool m_needsNodePropertyDisplay = true;
    };
} 
