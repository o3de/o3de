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

#include <Components/Slots/SlotConnectionPin.h>

namespace GraphCanvas
{
    class DataSlotConnectionPin
        : public SlotConnectionPin
    {
    public:
        AZ_RTTI(DataSlotConnectionPin, "{704E0929-B231-4E24-BD6F-C61950F62691}", SlotConnectionPin);
        AZ_CLASS_ALLOCATOR(DataSlotConnectionPin, AZ::SystemAllocator, 0);
        
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