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

#include <GraphCanvas/Components/NodePropertyDisplay/StringDataInterface.h>

#include "ScriptCanvasPropertyDataInterface.h"

namespace ScriptCanvasEditor
{
    class ScriptCanvasStringPropertyDataInterface
        : public ScriptCanvasPropertyDataInterface<GraphCanvas::StringDataInterface, ScriptCanvas::Data::StringType>
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasStringPropertyDataInterface, AZ::SystemAllocator, 0);
        ScriptCanvasStringPropertyDataInterface(const AZ::EntityId& nodeId, ScriptCanvas::TypedNodePropertyInterface<ScriptCanvas::Data::StringType>* propertyNodeInterface)
            : ScriptCanvasPropertyDataInterface(nodeId, propertyNodeInterface)
        {
        }
        
        ~ScriptCanvasStringPropertyDataInterface() = default;
        
        // StringDataInterface
        AZStd::string GetString() const override
        {
            return GetValue();
        }
        
        void SetString(const AZStd::string& value) override
        {
            SetValue(value);
        }
        ////
    };
}