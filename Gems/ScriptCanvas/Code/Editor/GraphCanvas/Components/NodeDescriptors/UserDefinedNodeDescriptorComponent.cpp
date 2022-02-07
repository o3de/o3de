/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include "UserDefinedNodeDescriptorComponent.h"

namespace ScriptCanvasEditor
{
    ///////////////////////////////////////
    // UserDefinedNodeDescriptorComponent
    ///////////////////////////////////////
    
    void UserDefinedNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        
        if (serializeContext)
        {
            serializeContext->Class<UserDefinedNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(2)
            ;
        }
    }
    
    UserDefinedNodeDescriptorComponent::UserDefinedNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::UserDefined)
    {
    }
}
