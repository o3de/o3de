/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>

#include "ClassMethodNodeDescriptorComponent.h"

namespace ScriptCanvasEditor
{
    ///////////////////////////////////////
    // ClassMethodNodeDescriptorComponent
    ///////////////////////////////////////
    
    void ClassMethodNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        
        if (serializeContext)
        {
            serializeContext->Class<ClassMethodNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(2)
            ;
        }
    }
    
    ClassMethodNodeDescriptorComponent::ClassMethodNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::ClassMethod)
    {
    }
}
