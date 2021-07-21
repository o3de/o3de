/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
