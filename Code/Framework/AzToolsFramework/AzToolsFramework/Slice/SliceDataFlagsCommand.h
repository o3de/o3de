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

#include <AzCore/Component/EntityId.h>
#include <AzCore/Serialization/DataPatch.h>
#include <AzToolsFramework/Undo/UndoSystem.h>

namespace AZ
{
    class SliceComponent;
}

namespace AzToolsFramework
{
    /**
     * Undoable command for setting a single data flag.
     * Data flags affect how inheritance works within a slice (see @ref AZ::DataPatch::Flag).
     */
    class SliceDataFlagsCommand
        : public UndoSystem::URSequencePoint
    {
    public:
        AZ_CLASS_ALLOCATOR(SliceDataFlagsCommand, AZ::SystemAllocator, 0);
        AZ_RTTI(SliceDataFlagsCommand, "{002F9CCE-3677-46FE-A2E8-FE406A002694}", UndoSystem::URSequencePoint);

        /** 
         * @param entityId The entity to set the data flag in.
         * @param targetDataAddress The address (relative to the entity) to set the data flag on.
         * @param dataFlag The flag to set.
         * @param flagOn Whether to turn the flag on or off.
         */
        SliceDataFlagsCommand(
            const AZ::EntityId& entityId,
            const AZ::DataPatch::AddressType& targetDataAddress,
            AZ::DataPatch::Flag dataFlag,
            bool flagOn,
            const AZStd::string& friendlyName,
            UndoSystem::URCommandID commandId=0);

        void Undo() override;
        void Redo() override;

        bool Changed() const override { return m_previousDataFlags != m_nextDataFlags; }

    protected:

        AZ::EntityId m_entityId;
        AZ::DataPatch::AddressType m_dataAddress;
        AZ::DataPatch::Flags m_previousDataFlags = 0;
        AZ::DataPatch::Flags m_nextDataFlags = 0;
    };

    /**
     * Undoable command for clearing any data flags at, or below, a given data address.
     * For example, removing data flags from a component and any data within it.
     * Data flags affect how inheritance works within a slice (see @ref AZ::DataPatch::Flag).
     */
    class ClearSliceDataFlagsBelowAddressCommand
        : public UndoSystem::URSequencePoint
    {
    public:
        AZ_CLASS_ALLOCATOR(ClearSliceDataFlagsBelowAddressCommand, AZ::SystemAllocator, 0);
        AZ_RTTI(ClearSliceDataFlagsBelowAddressCommand, "{3128AD23-40EB-4DEE-A16A-3FA04D94B573}", UndoSystem::URSequencePoint);

        /**
        * @param entityId The entity to whose data flags are being cleared.
        * @param targetDataAddress An address relative to the entity.
        * All data flags at, or below, this address will be cleared.
        */
        ClearSliceDataFlagsBelowAddressCommand(
            const AZ::EntityId& entityId,
            const AZ::DataPatch::AddressType& targetDataAddress,
            const AZStd::string& friendlyName,
            UndoSystem::URCommandID commandId=0);

        void Undo() override;
        void Redo() override;

        bool Changed() const override { return m_previousDataFlagsMap != m_nextDataFlagsMap; }

    protected:

        AZ::EntityId m_entityId;
        AZ::DataPatch::AddressType m_dataAddress;
        AZ::DataPatch::FlagsMap m_previousDataFlagsMap;
        AZ::DataPatch::FlagsMap m_nextDataFlagsMap;
    };
} // namespace AzToolsFramework