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
    class ExtenderSlotConnectionPin
        : public SlotConnectionPin
    {
    public:
        AZ_RTTI(ExtenderSlotConnectionPin, "{E495A7EA-98E2-4A7B-B776-097F2CBF6636}", SlotConnectionPin);
        AZ_CLASS_ALLOCATOR(ExtenderSlotConnectionPin, AZ::SystemAllocator);
        
        ExtenderSlotConnectionPin(const AZ::EntityId& slotId);
        ~ExtenderSlotConnectionPin();
        
        void OnRefreshStyle() override;
        
        void DrawConnectionPin(QPainter* painter, QRectF drawRect, bool isConnected) override;

    protected:

        void OnSlotClicked() override;
    };
}
