/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>

#include <Components/Connections/DataConnections/DataConnectionVisualComponent.h>
#include <Components/Connections/DataConnections/DataConnectionGraphicsItem.h>

namespace GraphCanvas
{
    //////////////////////////////////
    // DataConnectionVisualComponent
    //////////////////////////////////
    
    void DataConnectionVisualComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        
        if (serializeContext)
        {
            serializeContext->Class<DataConnectionVisualComponent, ConnectionVisualComponent>()
                ->Version(1)
            ;
        }
    }
    
    void DataConnectionVisualComponent::CreateConnectionVisual()
    {
        m_connectionGraphicsItem = AZStd::make_unique<DataConnectionGraphicsItem>(GetEntityId());
    }
}
