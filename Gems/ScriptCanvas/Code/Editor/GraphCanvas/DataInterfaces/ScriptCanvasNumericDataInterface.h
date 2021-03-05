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
        AZ_CLASS_ALLOCATOR(ScriptCanvasNumericDataInterface, AZ::SystemAllocator, 0);
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