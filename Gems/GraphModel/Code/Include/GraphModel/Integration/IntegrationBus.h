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

// AZ
#include <AzCore/EBus/EBus.h>

// Graph Canvas
#include <GraphCanvas/Editor/EditorTypes.h>

namespace GraphModelIntegration
{
    class GraphController;

    //! Bus functions that allow the GraphModel Integration system to callback to the client system.
    class IntegrationBusInterface : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Return the Graph Canvas EntityId for whichever Graph Canvas scene is active in the Editor
        virtual GraphCanvas::GraphId GetActiveGraphCanvasSceneId() const = 0;
        
        //! Notifies the client node graph system that the graph data has changed
        virtual void SignalSceneDirty(GraphCanvas::GraphId graphCanvasSceneId) = 0;
    };

    using IntegrationBus = AZ::EBus<IntegrationBusInterface>;
}
