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