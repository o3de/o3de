/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QColor>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Styling/StyleHelper.h>

namespace GraphCanvas
{
    class DataSlotConnectionPin;

    constexpr const char* k_ReferenceMimeType = "GraphCanvas::Data::ReferenceMimeType";
    constexpr const char* k_ValueMimeType = "GraphCanvas::DAta::ValueMimeType";

    class DataSlotUtils
    {
    public:
        static bool IsValueDataSlotType(DataSlotType dataSlotType)
        {
            return dataSlotType == DataSlotType::Value;
        }

        static bool IsValueDataReferenceType(DataSlotType dataSlotType)
        {
            return dataSlotType == DataSlotType::Reference;
        }
    };

    struct DataSlotConfiguration
        : public SlotConfiguration
    {
    public:
        AZ_RTTI(DataSlotConfiguration, "{76933814-A77A-4877-B72D-5DB0F541EDA5}", SlotConfiguration);
        AZ_CLASS_ALLOCATOR(DataSlotConfiguration, AZ::SystemAllocator);

        DataSlotConfiguration() = default;

        DataSlotConfiguration(const SlotConfiguration& slotConfiguration)
            : SlotConfiguration(slotConfiguration)
        {
        }

        bool                    m_canConvertTypes = true;
        DataSlotType            m_dataSlotType = DataSlotType::Value;
        DataValueType           m_dataValueType = DataValueType::Primitive;
        bool                    m_isUserAdded = false;

        AZ::Uuid                m_typeId;
        AZStd::vector<AZ::Uuid> m_containerTypeIds;
    };

    class DataSlotRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual bool ConvertToReference(bool isNewSlot = false) = 0;
        virtual bool CanConvertToReference(bool isNewSlot = false) const = 0;

        virtual bool ConvertToValue() = 0;
        virtual bool CanConvertToValue() const = 0;

        virtual DataSlotType GetDataSlotType() const = 0;
        virtual DataValueType GetDataValueType() const = 0;

        virtual AZ::Uuid GetDataTypeId() const = 0;
        virtual void SetDataTypeId(AZ::Uuid typeId) = 0;

        virtual bool IsUserSlot() const = 0;

        virtual const Styling::StyleHelper* GetDataColorPalette() const = 0;

        virtual size_t GetContainedTypesCount() const = 0;
        virtual AZ::Uuid GetContainedTypeId(size_t index) const = 0;
        virtual const Styling::StyleHelper* GetContainedTypeColorPalette(size_t index) const = 0;

        virtual void SetDataAndContainedTypeIds(AZ::Uuid typeId, const AZStd::vector<AZ::Uuid>& typeIds, DataValueType valueType) = 0;
    };

    using DataSlotRequestBus = AZ::EBus<DataSlotRequests>;
    
    class DataSlotNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnVariableAssigned([[maybe_unused]] const AZ::EntityId& variableId) {}
        virtual void OnDataSlotTypeChanged([[maybe_unused]] const DataSlotType& dataSlotType) {}
        virtual void OnDisplayTypeChanged([[maybe_unused]] const AZ::Uuid& dataType, [[maybe_unused]] const AZStd::vector<AZ::Uuid>& typeIds) {}
        
        virtual void OnDragDropStateStateChanged([[maybe_unused]] const DragDropState& dragDropState) { }
    };
    
    using DataSlotNotificationBus = AZ::EBus<DataSlotNotifications>;

    class DataSlotLayoutRequests
        : public AZ::EBusTraits
    {
    public:
        // BusId here is the specific slot that we want to make requests to.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        
        virtual const DataSlotConnectionPin* GetConnectionPin() const = 0;

        virtual void UpdateDisplay() = 0;

        virtual QRectF GetWidgetSceneBoundingRect() const = 0;
    };

    using DataSlotLayoutRequestBus = AZ::EBus<DataSlotLayoutRequests>;

    //! Actions that are keyed off of the Node, but should be handled by the individual slots
    class NodeDataSlotRequests : public AZ::EBusTraits
    {
    public:
        // The id here is the Node that the slot belongs to.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using BusIdType = AZ::EntityId;

        //! Signals that the slots should try and recreate all of the slot property displays.
        virtual void RecreatePropertyDisplay() = 0;
    };

    using NodeDataSlotRequestBus = AZ::EBus<NodeDataSlotRequests>;

    class DataSlotDragDropInterface
    {
    public:

        virtual ~DataSlotDragDropInterface() = default;

        virtual AZ::Outcome<DragDropState> OnDragEnterEvent(QGraphicsSceneDragDropEvent* dragDropEvent) = 0;
        virtual void OnDragLeaveEvent(QGraphicsSceneDragDropEvent* dragDropEvent) = 0;
        virtual void OnDropEvent(QGraphicsSceneDragDropEvent* dropEvent) = 0;
        virtual void OnDropCancelled() = 0;
    };
}
