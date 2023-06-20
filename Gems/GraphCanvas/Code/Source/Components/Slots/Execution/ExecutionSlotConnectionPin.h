/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Components/Slots/SlotConnectionPin.h>

namespace GraphCanvas
{
    class ExecutionSlotConnectionPin
        : public SlotConnectionPin
    {
    public:
        AZ_RTTI(ExecutionSlotConnectionPin, "{3D4D5623-133A-4C0B-8D5F-D50813B69031}", SlotConnectionPin);
        AZ_CLASS_ALLOCATOR(ExecutionSlotConnectionPin, AZ::SystemAllocator);
        
        ExecutionSlotConnectionPin(const AZ::EntityId& slotId);
        ~ExecutionSlotConnectionPin();
        
        void OnRefreshStyle() override;
        
        void DrawConnectionPin(QPainter* painter, QRectF drawRect, bool isConnected) override;
        
    private:

        Styling::StyleHelper m_connectedStyle;
    };
}
