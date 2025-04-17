/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/utils.h>

#include <ScriptCanvas/Core/Contracts/SlotTypeContract.h>
#include <ScriptCanvas/Core/Datum.h>

namespace ScriptCanvas
{
    enum class CombinedSlotType : AZ::s32
    {
        None = 0,

        ExecutionIn,
        ExecutionOut,
        DataIn,
        DataOut,
        LatentOut,
    };

    enum class ConnectionType : AZ::s32
    {
        Unknown = 0,
        Input,
        Output
    };

    enum class SlotTypeDescriptor : AZ::s32
    {
        Unknown = 0,

        Execution,
        Data
    };

    constexpr bool IsExecution(CombinedSlotType slotType)
    {
        return slotType == CombinedSlotType::ExecutionIn
            || slotType == CombinedSlotType::ExecutionOut
            || slotType == CombinedSlotType::LatentOut;
    }

    constexpr bool IsExecutionOut(CombinedSlotType slotType)
    {
        return slotType == CombinedSlotType::ExecutionOut || slotType == CombinedSlotType::LatentOut;
    }

    constexpr bool IsData(CombinedSlotType slotType)
    {
        return slotType == CombinedSlotType::DataIn || slotType == CombinedSlotType::DataOut;
    }

    class SlotTypeUtils
    {
    public:
        static AZStd::pair<ConnectionType, SlotTypeDescriptor> BreakApartSlotType(CombinedSlotType slotType);
    };

    enum class DynamicDataType : AZ::s32
    {
        None = 0,

        Value,
        Container,
        Any
    };

    struct SlotDescriptor
    {
        AZ_CLASS_ALLOCATOR(SlotDescriptor, AZ::SystemAllocator);
        AZ_TYPE_INFO(SlotDescriptor, "{FBF1C3A7-AA74-420F-BBE4-29F78D6EA262}");

        constexpr SlotDescriptor() = default;

        SlotDescriptor(CombinedSlotType slotType)
        {
            AZStd::pair<ConnectionType, SlotTypeDescriptor> brokenDescriptor = SlotTypeUtils::BreakApartSlotType(slotType);

            m_connectionType = brokenDescriptor.first;
            m_slotType = brokenDescriptor.second;
        }

        constexpr SlotDescriptor(ConnectionType connectionType, SlotTypeDescriptor slotType)
            : m_connectionType(connectionType)
            , m_slotType(slotType)
        {
        }

        bool CanConnectTo(const SlotDescriptor& slotDescriptor) const
        {
            bool validConnection = true;

            if (m_slotType != slotDescriptor.m_slotType)
            {
                validConnection = false;
            }
            else
            {
                if (m_connectionType == ConnectionType::Input)
                {
                    if (slotDescriptor.m_connectionType != ConnectionType::Output)
                    {
                        validConnection = false;
                    }
                }
                else if (m_connectionType == ConnectionType::Output)
                {
                    if (slotDescriptor.m_connectionType != ConnectionType::Input)
                    {
                        validConnection = false;
                    }
                }
            }

            return validConnection;
        }

        constexpr bool operator==(const SlotDescriptor& other) const
        {
            return m_connectionType == other.m_connectionType
                && m_slotType == other.m_slotType;
        }

        constexpr bool operator!=(const SlotDescriptor& other) const
        {
            return !((*this) == other);
        }

        constexpr bool IsInput() const
        {
            return m_connectionType == ConnectionType::Input;
        }

        constexpr bool IsOutput() const
        {
            return m_connectionType == ConnectionType::Output;
        }

        constexpr bool IsData() const
        {
            return m_slotType == SlotTypeDescriptor::Data;
        }

        constexpr bool IsExecution() const
        {
            return m_slotType == SlotTypeDescriptor::Execution;
        }

        constexpr bool IsValid() const
        {
            return m_connectionType != ConnectionType::Unknown
                && m_slotType != SlotTypeDescriptor::Unknown;
        }

        ConnectionType m_connectionType = ConnectionType::Unknown;
        SlotTypeDescriptor m_slotType   = SlotTypeDescriptor::Unknown;
    };

    template<ConnectionType ConnectionName = ConnectionType::Unknown, SlotTypeDescriptor SlotTypeName = SlotTypeDescriptor::Unknown>
    struct DescriptorHelper
        : public SlotDescriptor
    {
        constexpr DescriptorHelper()
            : SlotDescriptor(ConnectionName, SlotTypeName)
        {
        }
    };

    ////
    // Predefines for ease of use
    struct SlotDescriptors
    {    
    public:
        struct ExecutionInDescriptor : DescriptorHelper<ConnectionType::Input, SlotTypeDescriptor::Execution> {};
        struct ExecutionOutDescriptor : DescriptorHelper<ConnectionType::Output, SlotTypeDescriptor::Execution> {};
        struct DataInDescriptor : DescriptorHelper<ConnectionType::Input, SlotTypeDescriptor::Data> {};
        struct DataOutDescriptor : DescriptorHelper<ConnectionType::Output, SlotTypeDescriptor::Data> {};

        static constexpr ExecutionInDescriptor ExecutionIn()
        {
            return ExecutionInDescriptor();
        }

        static constexpr ExecutionOutDescriptor ExecutionOut()
        {
            return ExecutionOutDescriptor();
        }

        static constexpr DataInDescriptor DataIn()
        {
            return DataInDescriptor();
        }

        static constexpr DataOutDescriptor DataOut()
        {
            return DataOutDescriptor();
        }
    };
    ////

    struct SlotConfiguration
    {
    protected:        
        SlotConfiguration(SlotTypeDescriptor slotType);

    public:
        AZ_CLASS_ALLOCATOR(SlotConfiguration, AZ::SystemAllocator);
        AZ_RTTI(SlotConfiguration, "{C169C86A-378F-4263-8B8D-C40D51631ECF}");

        virtual ~SlotConfiguration() = default;

        void SetConnectionType(ConnectionType connectionType);
        ConnectionType GetConnectionType() const { return m_slotDescriptor.m_connectionType; }

        const SlotDescriptor& GetSlotDescriptor() const { return m_slotDescriptor; }

        AZStd::string m_name;
        AZStd::string m_toolTip;

        bool m_isVisible = true;
        bool m_isLatent = false;
        bool m_isUserAdded = false;
        bool m_canHaveInputField = true;
        bool m_isNameHidden = false;

        // Enabling this attribute on an execution slot will cause it to automatically make a "behind the scenes"
        // connection to nodes connected by other slots of the same connection type as this slot
        bool m_createsImplicitConnections = false;

        AZStd::vector<ContractDescriptor> m_contractDescs;
        bool m_addUniqueSlotByNameAndType = true; // Only adds a new slot if a slot with the supplied name and CombinedSlotType does not exist on the node

        // Specifies the Id the slot will use. Generally necessary only in undo/redo case with dynamically added
        // slots to preserve data integrity
        SlotId m_slotId;

        AZStd::string m_displayGroup;

    private:
        SlotDescriptor m_slotDescriptor;
    };

    struct ExecutionSlotConfiguration
        : public SlotConfiguration
    {
        AZ_CLASS_ALLOCATOR(ExecutionSlotConfiguration, AZ::SystemAllocator);
        AZ_RTTI(ExecutionSlotConfiguration, "{F2785E7D-635F-4C94-BAB2-F09F8FB2B7CF}", SlotConfiguration);

        ExecutionSlotConfiguration()
            : SlotConfiguration(SlotTypeDescriptor::Execution)
        {
        }

        ExecutionSlotConfiguration(AZStd::string_view name, ConnectionType connectionType, AZStd::string_view toolTip = {})
            : SlotConfiguration(SlotTypeDescriptor::Execution)
        {
            m_name = name;
            m_toolTip = toolTip;

            SetConnectionType(connectionType);
        }
    };

    struct DataSlotConfiguration
        : public SlotConfiguration
    {
        AZ_CLASS_ALLOCATOR(DataSlotConfiguration, AZ::SystemAllocator);
        AZ_RTTI(DataSlotConfiguration, "{9411A82E-EB3E-4235-9DDA-12EF6C9ECB1D}", SlotConfiguration);

        DataSlotConfiguration()
            : SlotConfiguration(SlotTypeDescriptor::Data)
        {
        }

        DataSlotConfiguration(Datum&& datum);
        DataSlotConfiguration(Data::Type dataType);
        DataSlotConfiguration(Data::Type dataType, AZStd::string name, ConnectionType connectionType);

        template<class DataType>
        void SetDefaultValue(DataType defaultValue)
        {
            m_datum.SetAZType<DataType>();
            m_datum.Set<DataType>(defaultValue);
        }

        template<class DataType>
        void SetAZType()
        {
            m_datum.SetAZType<DataType>();
        }

        void SetType(Data::Type dataType);
        void SetType(const AZ::BehaviorParameter& typeDesc);

        void ConfigureDatum(Datum&& datum)
        {
            m_datum.ReconfigureDatumTo(AZStd::move(datum));
        }

        void CopyTypeAndValueFrom(const Datum& source);

        void DeepCopyFrom(const Datum& source);

        const Datum& GetDatum() const
        {
            return m_datum;
        }

    private:
        Datum m_datum;
    };

    struct DynamicDataSlotConfiguration
        : public SlotConfiguration
    {
        AZ_CLASS_ALLOCATOR(DynamicDataSlotConfiguration, AZ::SystemAllocator);
        AZ_RTTI(DynamicDataSlotConfiguration, "{64BB0D10-D776-4D28-AF33-065530A95310}", SlotConfiguration);

        DynamicDataSlotConfiguration()
            : SlotConfiguration(SlotTypeDescriptor::Data)
        {
        }

        AZ::Crc32       m_dynamicGroup = AZ::Crc32();
        DynamicDataType m_dynamicDataType = DynamicDataType::None;
        Data::Type m_displayType = Data::Type::Invalid();
    };
}
