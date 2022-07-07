/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Color.h>

#include <GraphCanvas/Components/NodePropertyDisplay/VectorDataInterface.h>

#include "ScriptCanvasDataInterface.h"

namespace ScriptCanvasEditor
{
    class ScriptCanvasColorDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::VectorDataInterface>
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasColorDataInterface, AZ::SystemAllocator, 0);
        ScriptCanvasColorDataInterface(const AZ::EntityId& nodeId, const ScriptCanvas::SlotId& slotId)
            : ScriptCanvasDataInterface(nodeId, slotId)
        {
        }
        
        ~ScriptCanvasColorDataInterface() = default;
        
        int GetElementCount() const override
        {
            return 4;
        }
        
        double GetValue(int index) const override
        {
            if (index < GetElementCount())
            {
                const ScriptCanvas::Datum* object = GetSlotObject();

                if (object)
                {
                    const AZ::Color* retVal = object->GetAs<AZ::Color>();

                    if (retVal)
                    {
                        float resultValue = 0.0f;

                        switch (index)
                        {
                        case 0:
                            resultValue = aznumeric_cast<float>(static_cast<int>(static_cast<float>(retVal->GetR()) * GetMaximum(index)));
                            break;
                        case 1:
                            resultValue = aznumeric_cast<float>(static_cast<int>(static_cast<float>(retVal->GetG()) * GetMaximum(index)));
                            break;
                        case 2:
                            resultValue = aznumeric_cast<float>(static_cast<int>(static_cast<float>(retVal->GetB()) * GetMaximum(index)));
                            break;
                        case 3:
                            resultValue = aznumeric_cast<float>(static_cast<int>(static_cast<float>(retVal->GetA()) * GetMaximum(index)));
                            break;
                        default:
                            break;
                        }
                        
                        return aznumeric_cast<double>(static_cast<float>(resultValue));
                    }
                }
            }
            
            return 0.0;
        }

        void SetValue(int index, double value) override
        {
            if (index < GetElementCount())
            {
                ScriptCanvas::ModifiableDatumView datumView;
                ModifySlotObject(datumView);

                if (datumView.IsValid())
                {                    
                    AZ::Color currentColor = (*datumView.GetAs<AZ::Color>());
                    
                    switch (index)
                    {
                    case 0:
                        currentColor.SetR(aznumeric_cast<float>(value / GetMaximum(index)));
                        break;
                    case 1:
                        currentColor.SetG(aznumeric_cast<float>(value / GetMaximum(index)));
                        break;
                    case 2:
                        currentColor.SetB(aznumeric_cast<float>(value / GetMaximum(index)));
                        break;
                    case 3:
                        currentColor.SetA(aznumeric_cast<float>(value / GetMaximum(index)));
                        break;
                    default:
                        break;
                    }

                    datumView.SetAs(currentColor);
                    
                    PostUndoPoint();
                    PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
                }
            }
        }        
        
        const char* GetLabel(int index) const override
        {
            switch (index)
            {
            case 0:
                return "R";
            case 1:
                return "G";
            case 2:
                return "B";
            case 3:
                return "A";
            default:
                return "???";
            }
        }
        
        AZStd::string GetStyle() const override
        {
            return "vectorized";
        }
        
        AZStd::string GetElementStyle(int index) const override
        {
            return AZStd::string::format("vector_%i", index);
        }

        int GetDecimalPlaces([[maybe_unused]] int index) const override
        {
            return 0;
        }
        
        double GetMinimum([[maybe_unused]] int index) const override
        {
            return 0.0;
        }
        
        double GetMaximum([[maybe_unused]] int index) const override
        {
            return 255.0;
        }
    };
}
