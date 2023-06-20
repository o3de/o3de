/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/NodePropertyDisplay/ComboBoxDataInterface.h>
#include <GraphCanvas/Components/NodePropertyDisplay/NumericDataInterface.h>

#include "ScriptCanvasDataInterface.h"

namespace ScriptCanvasEditor
{
    // Used for general numeric input.
    class ScriptCanvasNumericDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::NumericDataInterface>        
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasNumericDataInterface, AZ::SystemAllocator);
        ScriptCanvasNumericDataInterface(const AZ::EntityId& nodeId, const ScriptCanvas::SlotId& slotId)
            : ScriptCanvasDataInterface(nodeId, slotId)
        {
        }
        
        ~ScriptCanvasNumericDataInterface() = default;
        
        // NumericDataInterface
        double GetNumber() const override
        {
            const ScriptCanvas::Datum* object = GetSlotObject();            

            if (object)
            {
                const double* retVal = object->GetAs<double>();

                if (retVal)
                {
                    return (*retVal);
                }
            }
            
            return 0.0;
        }
        
        void SetNumber(double value) override
        {
            ScriptCanvas::ModifiableDatumView datumView;
            ModifySlotObject(datumView);

            if (datumView.IsValid())
            {
                datumView.SetAs(value);

                PostUndoPoint();
                PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
            }
        }
        ////
    };
}
