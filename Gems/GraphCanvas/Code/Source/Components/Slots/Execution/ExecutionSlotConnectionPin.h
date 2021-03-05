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
    class ExecutionSlotConnectionPin
        : public SlotConnectionPin
    {
    public:
        AZ_RTTI(ExecutionSlotConnectionPin, "{3D4D5623-133A-4C0B-8D5F-D50813B69031}", SlotConnectionPin);
        AZ_CLASS_ALLOCATOR(ExecutionSlotConnectionPin, AZ::SystemAllocator, 0);
        
        ExecutionSlotConnectionPin(const AZ::EntityId& slotId);
        ~ExecutionSlotConnectionPin();
        
        void OnRefreshStyle() override;
        
        void DrawConnectionPin(QPainter* painter, QRectF drawRect, bool isConnected) override;
        
    private:

        Styling::StyleHelper m_connectedStyle;
    };
}