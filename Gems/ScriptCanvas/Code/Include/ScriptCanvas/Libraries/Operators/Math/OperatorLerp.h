/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/unordered_set.h>

#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/GraphBus.h>


#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorLerp.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
#define LERPABLE_TYPES { Data::Type::Number(), Data::Type::Vector2(), Data::Type::Vector3(), Data::Type::Vector4() }

        //! Deprecated: see NodeableNodeOverloadedLerp
        class LerpBetween
            : public Node
        {
        public:

            SCRIPTCANVAS_NODE(LerpBetween);

            Data::Type m_displayType;

            // Data Input SlotIds
            SlotId m_startSlotId;
            SlotId m_stopSlotId;
            SlotId m_speedSlotId;
            SlotId m_maximumTimeSlotId;
            ////

            // Data Output SlotIds
            SlotId m_stepSlotId;
            SlotId m_percentSlotId;
            ////

            ////

            LerpBetween() = default;
            ~LerpBetween() override = default;

            void OnInit() override;
            void OnConfigured() override;

        private:

            void SetupInternalSlotReferences();
            bool IsGroupConnected() const;
            bool IsInGroup(const SlotId& slotId) const;

            AZStd::unordered_set< SlotId > m_groupedSlotIds;

            float m_duration;
            float m_counter;

            Datum m_startDatum;
            Datum m_differenceDatum;
        };

#undef LERPABLE_TYPES
    }
}
