/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

#include <GraphCanvas/Editor/EditorTypes.h>

namespace ScriptCanvasEditor
{
    class GraphValidatorDockWidgetNotifications : public AZ::EBusTraits
    {
    public:        
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = GraphCanvas::EditorId;
        
        virtual void OnResultsChanged(int erorrCount, int warningCount) = 0;
    };

    using GraphValidatorDockWidgetNotificationBus = AZ::EBus<GraphValidatorDockWidgetNotifications>;    
}
