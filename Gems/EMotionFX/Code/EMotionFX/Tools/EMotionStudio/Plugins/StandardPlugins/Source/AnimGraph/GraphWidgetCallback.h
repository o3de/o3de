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

#ifndef __EMSTUDIO_GRAPHWIDGETCALLBACK_H
#define __EMSTUDIO_GRAPHWIDGETCALLBACK_H

// include required headers
#include <MCore/Source/StandardHeaders.h>
#include "../StandardPluginsConfig.h"
#include <QPainter>


namespace EMStudio
{
    // forward declarations
    class NodeGraphWidget;

    // the graph widget callback base class
    class GraphWidgetCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(GraphWidgetCallback, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        // constructor and destructor
        GraphWidgetCallback(NodeGraphWidget* graphWidget)   { mGraphWidget = graphWidget; }
        virtual ~GraphWidgetCallback()                      {}

        virtual void DrawOverlay(QPainter& painter) = 0;

    protected:
        NodeGraphWidget* mGraphWidget;
    };
}   // namespace EMStudio

#endif
