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
    class ExtenderSlotConnectionPin
        : public SlotConnectionPin
    {
    public:
        AZ_RTTI(ExtenderSlotConnectionPin, "{E495A7EA-98E2-4A7B-B776-097F2CBF6636}", SlotConnectionPin);
        AZ_CLASS_ALLOCATOR(ExtenderSlotConnectionPin, AZ::SystemAllocator, 0);
        
        ExtenderSlotConnectionPin(const AZ::EntityId& slotId);
        ~ExtenderSlotConnectionPin();
        
        void OnRefreshStyle() override;
        
        void DrawConnectionPin(QPainter* painter, QRectF drawRect, bool isConnected) override;

    protected:

        void OnSlotClicked() override;
    };
}