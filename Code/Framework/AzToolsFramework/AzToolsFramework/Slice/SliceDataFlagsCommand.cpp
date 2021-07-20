/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SliceDataFlagsCommand.h"
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace AzToolsFramework
{
    SliceDataFlagsCommand::SliceDataFlagsCommand(
        const AZ::EntityId& entityId,
        const AZ::DataPatch::AddressType& targetDataAddress,
        AZ::DataPatch::Flag dataFlag,
        bool additive,
        const AZStd::string& friendlyName,
        UndoSystem::URCommandID commandId/*=0*/)
        : URSequencePoint(friendlyName, commandId)
        , m_entityId(entityId)
        , m_dataAddress(targetDataAddress)
    {

        if (AZ::SliceComponent* rootSlice = GetEntityRootSlice(m_entityId))
        {
            m_previousDataFlags = rootSlice->GetEntityDataFlagsAtAddress(m_entityId, m_dataAddress);
        }
        else
        {
            AZ_Warning("Undo", false, "Cannot find slice containing entity ID %s", m_entityId.ToString().c_str());
        }

        if (additive)
        {
            m_nextDataFlags = m_previousDataFlags | dataFlag;
        }
        else
        {
            m_nextDataFlags = m_previousDataFlags & ~dataFlag;
        }

        Redo();
    }

    void SliceDataFlagsCommand::Undo()
    {
        if (AZ::SliceComponent* rootSlice = GetEntityRootSlice(m_entityId))
        {
            rootSlice->SetEntityDataFlagsAtAddress(m_entityId, m_dataAddress, m_previousDataFlags);
        }
    }

    void SliceDataFlagsCommand::Redo()
    {
        if (AZ::SliceComponent* rootSlice = GetEntityRootSlice(m_entityId))
        {
            rootSlice->SetEntityDataFlagsAtAddress(m_entityId, m_dataAddress, m_nextDataFlags);
        }
    }

    ClearSliceDataFlagsBelowAddressCommand::ClearSliceDataFlagsBelowAddressCommand(
        const AZ::EntityId& entityId,
        const AZ::DataPatch::AddressType& targetDataAddress,
        const AZStd::string& friendlyName,
        UndoSystem::URCommandID commandId/*=0*/)
        : URSequencePoint(friendlyName, commandId)
        , m_entityId(entityId)
        , m_dataAddress(targetDataAddress)
    {
        if (AZ::SliceComponent* rootSlice = GetEntityRootSlice(m_entityId))
        {
            m_previousDataFlagsMap = rootSlice->GetEntityDataFlags(m_entityId);

            // m_nextDataFlagsMap is a copy of the map, but without entries at or below m_dataAddress
            for (const auto& addressFlagsPair : m_previousDataFlagsMap)
            {
                const AZ::DataPatch::AddressType& address = addressFlagsPair.first;
                if (m_dataAddress.size() <= address.size())
                {
                    if (AZStd::equal(m_dataAddress.begin(), m_dataAddress.end(), address.begin()))
                    {
                        continue;
                    }
                }

                m_nextDataFlagsMap.emplace(addressFlagsPair);
            }
        }
        else
        {
            AZ_Warning("Undo", false, "Cannot find slice containing entity ID %s", m_entityId.ToString().c_str());
        }

        Redo();
    }

    void ClearSliceDataFlagsBelowAddressCommand::Undo()
    {
        if (AZ::SliceComponent* rootSlice = GetEntityRootSlice(m_entityId))
        {
            rootSlice->SetEntityDataFlags(m_entityId, m_previousDataFlagsMap);
        }
    }

    void ClearSliceDataFlagsBelowAddressCommand::Redo()
    {
        if (AZ::SliceComponent* rootSlice = GetEntityRootSlice(m_entityId))
        {
            rootSlice->SetEntityDataFlags(m_entityId, m_nextDataFlagsMap);
        }
    }

} // namespace AzToolsFramework
