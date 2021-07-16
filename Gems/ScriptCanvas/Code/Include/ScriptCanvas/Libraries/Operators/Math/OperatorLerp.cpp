/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorLerp.h"

#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Utils/VersionConverters.h>

#include <Include/ScriptCanvas/Data/NumericData.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        void LerpBetween::OnInit()
        {
            SetupInternalSlotReferences();

            ///////// Version Conversion to Dynamic Grouped based operators
            SlotId stepId = LerpBetweenProperty::GetStepSlotId(this);

            if (stepId.IsValid())
            {
                for (const SlotId& slotId : { m_startSlotId, m_stopSlotId, m_speedSlotId, stepId })
                {
                    Slot* slot = GetSlot(slotId);

                    if (slot->IsDynamicSlot() && slot->GetDynamicGroup() == AZ::Crc32())
                    {
                        SetDynamicGroup(slotId, AZ::Crc32("LerpGroup"));
                    }
                }
            }
            ////////

            //////// Version Conversion will remove the Display type in a few revisions
            if (m_displayType.IsValid())
            {
                Data::Type displayType = GetDisplayType(AZ::Crc32("LerpGroup"));

                if (!displayType.IsValid())
                {
                    SetDisplayType(AZ::Crc32("LerpGroup"), displayType);
                }
            }
            ////////
        }

        void LerpBetween::OnConfigured()
        {
            SetupInternalSlotReferences();
        }

        void LerpBetween::SetupInternalSlotReferences()
        {
            m_startSlotId = LerpBetweenProperty::GetStartSlotId(this);
            m_stopSlotId = LerpBetweenProperty::GetStopSlotId(this);
            m_maximumTimeSlotId = LerpBetweenProperty::GetMaximumDurationSlotId(this);
            m_speedSlotId = LerpBetweenProperty::GetSpeedSlotId(this);

            m_stepSlotId = LerpBetweenProperty::GetStepSlotId(this);
            m_percentSlotId = LerpBetweenProperty::GetPercentSlotId(this);
        }

        bool LerpBetween::IsGroupConnected() const
        {
            bool hasConnections = false;
            for (auto slotId : m_groupedSlotIds)
            {
                if (IsConnected(slotId))
                {
                    hasConnections = true;
                    break;
                }
            }

            return hasConnections;
        }

        bool LerpBetween::IsInGroup(const SlotId& slotId) const
        {
            return m_groupedSlotIds.count(slotId) > 0;
        }
    }
}
