/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>
#include <ScriptCanvas/Grammar/Primitives.h>

#include <ScriptCanvas/Grammar/DebugMap.h>

namespace ScriptCanvas
{
    namespace Grammar
    {
        void ReflectDebugSymbols(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<DebugDataSource>()
                    ->Version(0)
                    ->Field("sourceType", &DebugDataSource::m_sourceType)
                    ->Field("slotId", &DebugDataSource::m_slotId)
                    ->Field("slotDatumType", &DebugDataSource::m_slotDatumType)
                    ->Field("source", &DebugDataSource::m_source)
                    ;

                serializeContext->Class<DebugExecution>()
                    ->Version(0)
                    ->Field("sourceType", &DebugExecution::m_namedEndpoint)
                    ->Field("data", &DebugExecution::m_data)
                    ;

                serializeContext->Class<DebugSymbolMap>()
                    ->Version(0)
                    ->Field("ins", &DebugSymbolMap::m_ins)
                    ->Field("outs", &DebugSymbolMap::m_outs)
                    ->Field("returns", &DebugSymbolMap::m_returns)
                    ->Field("variables", &DebugSymbolMap::m_variables)
                    ;
            }
        }

        DebugDataSource::DebugDataSource()
            : m_sourceType(DebugDataSourceType::Internal)
            , m_slotId{}
            , m_slotDatumType{}
            , m_source(SlotId{})
        {}

        DebugDataSource::DebugDataSource(const Slot& selfSource, const Data::Type& ifInvalidType)
            : m_sourceType(DebugDataSourceType::SelfSlot)
            , m_slotId(selfSource.GetId())
            , m_slotDatumType(selfSource.GetDataType().IsValid() ? selfSource.GetDataType() : ifInvalidType)
            , m_source(SlotId{})
        {
            AZ_Assert(m_slotDatumType.IsValid(), "data type must be valid at compile time");
        }

        DebugDataSource::DebugDataSource(const SlotId& slotId, const Data::Type& originalSlotType, const SlotId& source)
            : m_sourceType(DebugDataSourceType::OtherSlot)
            , m_slotId(slotId)
            , m_slotDatumType(originalSlotType)
            , m_source(source)
        {
            AZ_Assert(m_slotDatumType.IsValid(), "data type must be valid at compile time");
        }

        DebugDataSource::DebugDataSource(const SlotId& slotId, const Data::Type& originalSlotType, const VariableId& source)
            : m_sourceType(DebugDataSourceType::Variable)
            , m_slotId(slotId)
            , m_slotDatumType(originalSlotType)
            , m_source(source)
        {
            AZ_Assert(m_slotDatumType.IsValid(), "data type must be valid at compile time");
        }

        DebugDataSource DebugDataSource::FromInternal()
        {
            return DebugDataSource();
        }

        DebugDataSource DebugDataSource::FromInternal(const Data::Type& dataType)
        {
            DebugDataSource debugData;
            debugData.m_sourceType = DebugDataSourceType::Internal;
            debugData.m_slotDatumType = dataType;
            return debugData;
        }

        DebugDataSource DebugDataSource::FromSelfSlot(const Slot& localSource)
        {
            return FromSelfSlot(localSource, localSource.GetDataType());
        }

        DebugDataSource DebugDataSource::FromSelfSlot(const Slot& localSource, const Data::Type& ifInvalidType)
        {
            return DebugDataSource(localSource, ifInvalidType);
        }

        DebugDataSource DebugDataSource::FromOtherSlot(const SlotId& slotId, const Data::Type& originalType, const SlotId& source)
        {
            return DebugDataSource(slotId, originalType, source);
        }

        DebugDataSource DebugDataSource::FromVariable(const SlotId& slotId, const Data::Type& originalType, const VariableId& source)
        {
            return DebugDataSource(slotId, originalType, source);
        }

        DebugDataSource DebugDataSource::FromReturn(const Slot& slot, ExecutionTreeConstPtr execution, VariableConstPtr variable)
        {
            if (variable->m_sourceVariableId.IsValid())
            {
                return FromVariable(slot.GetId(), variable->m_datum.GetType(), variable->m_sourceVariableId);
            }
            else if (variable->m_sourceSlotId.IsValid())
            {
                if (variable->m_source == execution)
                {
                    return FromSelfSlot(slot);
                }
                else
                {
                    return FromOtherSlot(slot.GetId(), slot.GetDataType(), variable->m_sourceSlotId);
                }
            }
            else
            {
                // Technically internally provided. This condition could get parsed, but no ScriptCanvas node supports it, yet.
                return FromSelfSlot(slot);
            }
        }

    } 

} 
