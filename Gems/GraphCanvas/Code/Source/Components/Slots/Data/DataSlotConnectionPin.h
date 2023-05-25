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
    class DataSlotConnectionPin
        : public SlotConnectionPin
    {
    public:
        AZ_RTTI(DataSlotConnectionPin, "{704E0929-B231-4E24-BD6F-C61950F62691}", SlotConnectionPin);
        AZ_CLASS_ALLOCATOR(DataSlotConnectionPin, AZ::SystemAllocator);
        
        DataSlotConnectionPin(const AZ::EntityId& slotId);
        ~DataSlotConnectionPin();
        
        void OnRefreshStyle() override;
        
        void DrawConnectionPin(QPainter* painter, QRectF drawRect, bool isConnected) override; 
        
    private:

        void Setup(QPainter* painter, QPen& pen, QBrush& brush, QRectF drawRect, bool isConnected);

        const Styling::StyleHelper* m_colorPalette;
        AZStd::vector<const Styling::StyleHelper*> m_containerColorPalettes;
    };
}
