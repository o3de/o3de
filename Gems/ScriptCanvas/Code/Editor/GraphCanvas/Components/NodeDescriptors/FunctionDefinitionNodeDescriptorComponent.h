/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "NodelingDescriptorComponent.h"

#include <Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h>

#include <ScriptCanvas/Core/NodelingBus.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <GraphCanvas/Components/VisualBus.h>

namespace ScriptCanvasEditor
{
    class FunctionDefinitionNodeDescriptorComponent
        : public NodelingDescriptorComponent
        , GraphCanvas::VisualNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(FunctionDefinitionNodeDescriptorComponent, "{F433EC33-D8A7-40E0-97E7-B29C3C68323E}", NodelingDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);

        FunctionDefinitionNodeDescriptorComponent();
        ~FunctionDefinitionNodeDescriptorComponent() override = default;

        static bool RenameDialog(FunctionDefinitionNodeDescriptorComponent* descriptor);

    protected:

        void Activate() override;
        void Deactivate() override;

        // GraphCanvas::VisualNotificationBus::Handler...
        bool OnMouseDoubleClick(const QGraphicsSceneMouseEvent*) override;
        /////
    };
}
