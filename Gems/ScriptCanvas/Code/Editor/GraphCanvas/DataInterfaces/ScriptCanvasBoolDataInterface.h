/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/NodePropertyDisplay/BooleanDataInterface.h>

#include "ScriptCanvasDataInterface.h"

namespace ScriptCanvasEditor
{
    class ScriptCanvasBoolDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::BooleanDataInterface>
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasBoolDataInterface, AZ::SystemAllocator);
        ScriptCanvasBoolDataInterface(const AZ::EntityId& nodeId, const ScriptCanvas::SlotId& slotId)
            : ScriptCanvasDataInterface(nodeId, slotId)
        {
        }
        
        ~ScriptCanvasBoolDataInterface() = default;
        
        // BooleanDataInterface
        bool GetBool() const override
        {
            const ScriptCanvas::Datum* object = GetSlotObject();            

            if (object)
            {
                const bool* retVal = object->GetAs<bool>();

                if (retVal)
                {
                    return (*retVal);
                }
            }
            
            return false;
        }
        
        void SetBool(bool enabled) override
        {
            ScriptCanvas::ModifiableDatumView datumView;
            ModifySlotObject(datumView);

            if (datumView.IsValid())
            {
                datumView.SetAs(enabled);

                PostUndoPoint();
                PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
            }
        }
    };
}
