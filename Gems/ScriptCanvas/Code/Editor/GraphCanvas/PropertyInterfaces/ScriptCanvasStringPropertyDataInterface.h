/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(ScriptCanvasStringPropertyDataInterface, AZ::SystemAllocator);
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
